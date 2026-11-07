
# COS: Reduce memory pressure introduced by commit 8d32f84

## Objective

Reduce permanent RAM consumption, Berry heap usage and peak HTTP/HTTPS
memory consumption without sacrificing networking reliability or filesystem
functionality.

Prioritize changes that reduce memory consumption even when networking
is not actively being used.

Do not introduce arbitrary buffer size reductions or unsafe buffer sharing.

---

## P1. Remove redundant static HTTP buffers

Priority: Critical
Files:
- components/berry_lang/src/berry_network.cpp
- components/network/src/network_http.c
- components/network/include/network.h

### 1.1 Remove the duplicate idle response structure

Current implementation:

network_http.c:
- s_response
- s_idle

Both are complete network_http_response_t structures.

Each contains:
- A 513-byte URL buffer.
- 12 response headers.
- Header count, status and body metadata.

The second structure is unnecessary.

Changes:
- Remove s_idle.
- Remove the separate publish_idle() storage.
- Reuse s_response as the empty response representation.
- At the beginning of network_http_fetch(), zero s_response and set
  its body pointer to s_empty_body.
- Return &s_response as the response pointer.
- Continue populating the same structure for actual requests.

Preserve the existing API contract: response must remain non-NULL
when the caller supplies a response pointer, including on failure.

Expected saving: approximately 4.1 KB of static RAM.

### 1.2 Remove the duplicate Berry request body buffer

Current implementation in berry_network.cpp:

static char s_body[NETWORK_HTTP_BODY_MAX + 1];

The Berry binding copies the request body into s_body.

network_http_fetch() then copies it again into its own
network-owned s_req_body.

The second copy exists for a valid reason: the HTTP worker may
continue processing after the calling function times out.

Changes:
- Remove s_body.
- Read the body directly from the Berry string using be_tostring().
- Set request.body and request.body_len directly.
- Keep the original Berry string rooted on the VM stack until
  network_http_fetch() has copied it into its network-owned buffer.
- Release the temporary VM stack reference after the fetch call.

Keep:
- s_req_body in network_http.c.
- The existing request-size validation.
- The network component's copy-before-queue behavior.

Do NOT let the HTTP worker reference temporary Berry memory.

Expected saving: approximately 4.1 KB of static RAM.

### 1.3 Remove duplicate Berry request header storage

Current implementation:

berry_network.cpp:
- static network_http_header_t s_headers[NETWORK_HTTP_HEADER_MAX];

network_http.c:
- static network_http_header_t s_req_headers[NETWORK_HTTP_HEADER_MAX];

Both contain eight fixed-size header structures.

Changes:

Introduce a lightweight request-header view type in network.h:

    typedef struct
    {
        const char *name;
        const char *value;
    } network_http_request_header_t;

Change network_http_request_t.headers to use this new type.

Keep network_http_response_t.headers using the existing
network_http_header_t type because response headers need owned storage.

In berry_network.cpp:
- Remove static s_headers.
- Create a local array of network_http_request_header_t.
- Populate it with pointers to the original Berry header strings.
- Validate names and values exactly as before.
- Pass the array through network_http_request_t.

In network_http.c:
- Copy the request header strings into s_req_headers before
  placing the request on the worker queue.
- Preserve the current size validation and NUL termination.
- Keep s_req_headers as the single persistent owner used by the worker.

The Berry map must remain rooted until the network component
has completed its input copies.

Expected saving: approximately 2.4 KB of static RAM.

### 1.4 Preserve the remaining network-owned buffers

Do not remove these buffers in this optimization:

- s_req_url
- s_req_body
- s_req_headers
- s_resp_body
- s_response

These provide stable storage for the worker and response data.

The worker must never reference temporary caller-owned request
data after network_http_fetch() has returned.

### Expected result of P1

Remove approximately:

| Buffer | Expected saving |
|---|---:|
| s_idle | 4.1 KB |
| berry_network.cpp: s_body | 4.1 KB |
| berry_network.cpp: s_headers | 2.4 KB |
| Total | ~10.6 KB |

This is static RAM savings, not an estimate of runtime heap savings.

---

## P2. Make Berry JSON initialization lazy

Priority: Critical
File:
- components/berry_lang/src/berry_runtime.cpp

Current implementation:

berry_runtime_init() unconditionally executes:

    import json

This loads the JSON module for every Berry runtime instance,
even when no filesystem or networking JSON functionality is used.

Changes:

1. Remove unconditional JSON import from berry_runtime_init().
2. Introduce a shared ensure_json_module_loaded(bvm *vm) helper.
3. Check whether the global json module is already available.
4. Load the module only when a JSON-dependent operation is invoked.
5. Cache its initialization state per VM instance.
6. Reset the cached state whenever the VM is destroyed/recreated.

Call the helper before:
- fs.read_json()
- fs.write_json()
- net.fetch() when response_type == "json"

Do not load JSON for:
- Plain Berry application startup.
- fs.read_text().
- fs.write_text().
- HTTP requests with response_type == "text".
- Network status or Wi-Fi operations.

Preserve existing error handling if the module cannot be loaded.

Acceptance criteria:
- A normal Berry runtime must not load JSON automatically.
- JSON filesystem operations must still work.
- JSON HTTP parsing must still work.
- Repeated JSON operations must not reload the module.

---

## P3. Avoid retaining unnecessary raw JSON response bodies

Priority: Medium
File:
- components/berry_lang/src/berry_network.cpp
- frontend/src/cos-sdk/network.d.ts

Current behavior:

For response_type == "json", the implementation:
1. Copies the HTTP response into a Berry string.
2. Parses that string into a Berry JSON object.
3. Returns both the original body and parsed JSON object.
4. Retains both representations in the returned result map.

Changes:

Introduce an optional fetch parameter:

    include_body: bool

Behavior:
- Default: true, preserving backwards compatibility.
- When true: return the raw body as currently implemented.
- When false: return body as an empty string.
- Always return the parsed JSON object when requested and successful.

Update:
- known_fetch_field().
- Native option parsing.
- push_fetch_result().
- TypeScript declarations.
- SDK documentation.

For JSON responses where the caller only needs the parsed object,
allow:

    net.fetch(url, {
        "response_type": "json",
        "include_body": false
    })

Do not remove the default raw-body behavior because existing scripts
may depend on it.

Acceptance criteria:
- Existing scripts continue working unchanged.
- include_body=false avoids creating an additional Berry string
  containing the raw HTTP response.
- JSON parsing and error handling remain unchanged.

---

## P4. Measure and optimize the HTTP worker stack

Priority: Medium
File:
- components/network/src/network_http.c

Current implementation:

    #define HTTP_TASK_STACK 12288

The worker is created on the first HTTP request and remains alive
indefinitely.

Changes:

1. Add stack high-water-mark logging inside http_worker().
2. Measure stack usage during:
   - HTTP GET.
   - HTTP POST with a request body.
   - HTTPS GET.
   - HTTPS POST.
   - Maximum-size response.
   - Timeout and error scenarios.

3. Inspect the minimum remaining stack using
   uxTaskGetStackHighWaterMark().

4. Check the ESP-IDF port's stack-size units before calculating
   the new stack allocation.

5. Reduce HTTP_TASK_STACK only after measuring worst-case usage.

Target:
- Actual observed peak stack usage plus at least 25% safety margin.
- Do not blindly replace 12288 with an arbitrary smaller number.

Also inspect the caller task's stack. URL parsing in network_url.c
uses several relatively large local buffers and runs in the caller's
context, not necessarily the HTTP worker.

### Optional improvement: worker lifecycle

Currently http_worker() blocks indefinitely and never exits.

Investigate whether an idle-timeout worker shutdown is worthwhile.

Only implement this if:
- The worker remains unused for a meaningful period.
- Releasing its stack measurably improves available RAM.
- Queue, semaphore and task lifecycle synchronization can be
  implemented without races.

Do not add unnecessary task lifecycle complexity if reducing
the stack already solves the problem.

---

## P5. Measure HTTPS/TLS peak memory

Priority: Medium
Files:
- components/network/src/network_http.c
- sdkconfig.defaults

The commit introduces ESP-TLS, mbedTLS and the certificate bundle.

Do not disable HTTPS or certificate verification as a memory optimization.

Instead:

1. Record internal free heap before an HTTP request.
2. Record minimum free heap during an HTTP request.
3. Repeat for HTTPS.
4. Record free heap after esp_http_client_cleanup().
5. Compare largest free blocks before and after.

Check for temporary allocation pressure caused by:
- TLS handshake.
- Certificate verification.
- HTTP client buffers.
- Response reception.

Keep esp_http_client_cleanup() on all applicable success/error paths.

Only optimize TLS configuration after identifying an actual
measurable source of excessive memory consumption.

---

## P6. Avoid unnecessary filesystem changes

Priority: Low

The new filesystem functions contain temporary allocations,
but the reviewed code has explicit cleanup paths.

Do not rewrite the filesystem implementation as part of the
initial memory optimization.

Only investigate further if profiling shows filesystem operations
contribute significantly to heap pressure.

---

## P7. Memory profiling and regression tests

Priority: Critical validation

Use the following metrics:

    heap_caps_get_free_size(
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT
    );

    heap_caps_get_largest_free_block(
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT
    );

    heap_caps_get_minimum_free_size(
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT
    );

Record measurements at these checkpoints:

1. System startup, before Berry initialization.
2. Immediately after berry_runtime_init().
3. After opening a normal Berry application.
4. After executing fs.read_text().
5. After executing fs.read_json().
6. After the first HTTP GET.
7. After an HTTPS GET.
8. After a maximum-size HTTP response.
9. After Berry garbage collection.
10. After repeated network requests.

Also compare static memory:

    idf.py size
    idf.py size-components

Inspect .bss changes in berry_lang and network.

### Required regression tests

- Plain Berry applications work without JSON being loaded.
- Filesystem text operations work.
- Filesystem JSON operations work.
- HTTP text responses work.
- HTTP JSON responses work.
- HTTPS certificate verification still works.
- Large request bodies work.
- Maximum-size response handling remains correct.
- Timeout does not cause use-after-free.
- A timed-out request does not allow buffers to be overwritten
  while the worker is still using them.
- Repeated HTTP requests do not produce progressively decreasing
  free heap after garbage collection.
- HTTP worker stack remains safe under worst-case conditions.

---

## Final acceptance criteria

1. Recover approximately 10 KB of permanent static RAM from P1.
2. JSON module is no longer loaded during ordinary Berry startup.
3. Existing filesystem and networking API behavior remains compatible.
4. No request buffer lifetime or concurrency regressions.
5. HTTP and HTTPS work with adequate remaining heap.
6. No unexplained downward trend in free heap after repeated requests.
7. Largest free block is monitored, not just total free heap.

Implement P1 and P2 first. Measure again before proceeding with
P3 through P5.

Do not combine all optimizations into one large change.
Keep each priority group independently testable.
