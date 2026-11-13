What this repo does:
COS is firmware for an ESP32 CYD board (ESP32-2432S028R) with display/touch, a UART shell, LittleFS + optional SD storage, Wi-Fi/Bluetooth, Berry scripting, and an on-device HTTP API/WebSocket/static file server used by a web frontend. Build/deploy are driven by ESP-IDF/Make (`make build/flash/flash-data`) and a separate Vite frontend.

Load assumed: single embedded device, usually one local user at a time, with a small number of LAN clients (browser/WebSocket/API) when networking is enabled. RAM, flash, CPU, and socket counts are tight (see `sdkconfig.defaults`, `partitions.csv`, `components/http_server/src/http_server.c`).

## Must fix

### 1. **Default web credentials are public and active on first boot** (`components/http_server/src/http_auth.c:16-21`, `components/http_server/src/http_auth.c:49-56`, `README.md`)
- **What this is:** HTTP Basic auth defaults to username `cos` and password `cos12345`, loaded at startup if NVS has no saved credentials.
- **Problem:** Anyone on the same network who can reach the device can log in with known defaults and use authenticated APIs/upload/WebSocket console unless the owner changes credentials first.
- **Fix:** Do not ship a fixed shared password. Require setting credentials before enabling remote API access (or generate/store a per-device secret at first boot and show it once over local console/display), and document the bootstrap flow.
- **If we skip it:** A device connected to Wi-Fi can be taken over remotely with the documented default password (file read/write/upload/delete and interactive shell input over WebSocket).

### 2. **HTTP server starts unconditionally with remote-capable endpoints** (`main/main.c:100-110`, `components/http_server/src/http_server.c:51-80`, `components/websocket/src/websocket.c:247-267`)
- **What this is:** `app_main` always initializes HTTP server and WebSocket after network init, regardless of whether remote access was intentionally enabled.
- **Problem:** Network-facing services are exposed by default as soon as the device is reachable; combined with weak/default auth this increases attack surface on a constrained device.
- **Fix:** Start HTTP/WS only when explicitly enabled by config/command, or gate startup behind a deliberate "remote access" setting stored in NVS with safe defaults.
- **If we skip it:** Unexpected remote access on LAN, easier exploitation of auth/config mistakes, and avoidable RAM/socket/CPU use.

## Should fix

### 3. **Static file serving does not require authentication** (`components/http_server/src/http_server.c:72`, `components/http_server/src/http_static.c:181-246`)
- **What this is:** Static content is served from the 404 error handler path; API/upload handlers check `http_auth_check`, but `http_static_handler` does not.
- **Problem:** When static serving is enabled, files under the configured root can be fetched without credentials. If sensitive files are placed there (or root is misconfigured), they become public to anyone who can reach the device.
- **Fix:** Make static auth policy explicit: require auth by default (or provide a clear public/private mode), and enforce allowed roots.
- **If we skip it:** Accidental data exposure of web assets, configs, scripts, or other readable files served from the static root.

### 4. **Static path join trusts request URI segments without normalization** (`components/http_server/src/http_static.c:200-227`, `components/filesystem/src/vfs_path.c:158-196`)
- **What this is:** Static handler strips the query string, concatenates `s_root + clean_uri`, then resolves only `/flash` or `/sdcard` prefixes; `vfs_resolve_path` appends the remainder verbatim and does not normalize `.`/`..`.
- **Problem:** A crafted URI containing traversal-like segments may escape the intended static root depending on mount/VFS behavior. Upload paths are sanitized separately, but static reads use a different path flow.
- **Fix:** Canonicalize/normalize the joined virtual path (reuse `normalize_absolute` logic or equivalent) and reject any resolved path outside the configured root before `fopen/stat`.
- **If we skip it:** Possible read access outside the intended public directory on some filesystem/mount setups.

### 5. **Authenticated WebSocket input is fed directly to the shell** (`main/main.c:64-68`, `main/main.c:106-107`, `components/websocket/src/websocket.c:147-221`)
- **What this is:** WebSocket text frames are forwarded to `shell_feed_input`, effectively giving remote clients an interactive console stream.
- **Problem:** This is equivalent to remote shell access. With default/weak/shared credentials, compromise impact is full device control within shell/Berry capabilities.
- **Fix:** Treat WebSocket console as a privileged feature: require strong auth/bootstrap, allow disabling it independently of read-only APIs, and consider rate limiting/session logging.
- **If we skip it:** Credential theft or reuse leads directly to remote command execution on the device.

### 6. **Uploads can consume storage without an explicit size/quota guard in the handler** (`components/http_server/src/http_upload.c:39-170`, `components/http_server/src/http_multipart.c:105-310`)
- **What this is:** Multipart parsing streams file data to disk for the whole request body; the upload handler checks path/open/write errors but does not enforce a maximum upload size or free-space quota.
- **Problem:** An authenticated client (or anyone if auth is bypassed/misconfigured) can push large files until flash/SD is full, degrading or breaking logging, scripts, config writes, or future updates.
- **Fix:** Enforce configurable max content length/free-space checks before/during write, and fail early with a clear error.
- **If we skip it:** Storage exhaustion and denial of service; recovery may require manual cleanup or formatting.

### 7. **HTTP auth uses plain string comparison and stores credentials as blobs** (`components/http_server/src/http_auth.c:56-94`, `components/http_server/src/http_auth.c:120-135`)
- **What this is:** Credentials are compared with `strcmp` and persisted in NVS as raw username/password blobs.
- **Problem:** Direct password storage increases impact if NVS contents are extracted; timing side channels are less critical on a local embedded target but easy to avoid.
- **Fix:** Store a salted password verifier/hash (or at least avoid plaintext where feasible), and use constant-time comparison for secrets.
- **If we skip it:** Higher impact from device/NVS compromise and weaker credential hygiene.

### 8. **Multipart boundary/header parsing trusts `Content-Type` formatting narrowly** (`components/http_server/src/http_multipart.c:10-41`, `components/http_server/src/http_multipart.c:105-123`)
- **What this is:** Boundary extraction looks for `boundary=` and parses quoted/unquoted values, but header parsing for part names/filenames is simple substring search (`name="..."`, `filename="..."`).
- **Problem:** Some valid/odd multipart requests may be rejected or parsed incorrectly; unusual field ordering/casing/escaping can cause confusing upload failures or wrong metadata handling.
- **Fix:** Make parsing stricter and more predictable (case-insensitive header keys, explicit Content-Disposition parsing, reject malformed/ambiguous parts).
- **If we skip it:** Fragile uploads and hard-to-debug client compatibility issues; in worst cases malformed requests may bypass intended field handling.

## Nice to have

### 9. **`vfs_resolve_path` relies on callers for traversal normalization** (`components/filesystem/src/vfs_path.c:158-196`, `components/http_server/src/http_static.c:215-224`)
- **What this is:** Core path resolution maps virtual prefixes to mount points but does not sanitize/normalize the tail path.
- **Problem:** Every caller must remember to normalize untrusted input; current call sites are inconsistent.
- **Fix:** Centralize normalization/safety checks in VFS helpers so HTTP/static/script callers cannot forget.
- **If we skip it:** Repeated path-handling bugs as new features add file access.

### 10. **Repository carries build artifacts and logs in-tree** (`build/`, `_artifact_logs/`, `ci-artifacts-43188508915/`, `integration_test_esp_littlefs_run.log`, `sdkconfig`)
- **What this is:** Generated outputs and local artifact/log directories are present in the workspace root.
- **Problem:** They add noise, inflate search/index time, and make audits and cleanup harder; accidental edits/commits become more likely.
- **Fix:** Keep generated artifacts out of versioned source areas where possible, ensure ignore rules cover them, and document the canonical output locations.
- **If we skip it:** Slower reviews, confusing grep results, and risk of committing machine-specific state. (Lean opportunity.)

## Verdict:
Functional embedded firmware repo with clear structure, but remote access defaults and path-handling consistency need tightening before exposing the device beyond a trusted network.

Lean: ~large generated/output footprint removable from source review context, -0 dependencies possible (no dependency removal justified from this audit).

Not checked:
- Full runtime behavior on hardware (Wi-Fi/BT coexistence, touch/display UX, HTTPS/TLS edge cases beyond plan notes).
- Complete firmware lint/build/test execution in this environment (ESP-IDF/toolchain-dependent).
- Every component source file line-by-line; deep review focused on entry points, HTTP/WebSocket/upload/auth, filesystem/path, shell wiring, and repo-level config/docs.
- Frontend source internals beyond manifest/structure (TS/React-like IDE code not audited deeply in this pass).
- `managed_components/` and vendored third-party code (`components/esp_littlefs/src/littlefs`) except for presence/scope.

Skipped/limits: I changed no code. I used existing component review files only as context; findings above are based on direct repo inspection in this pass. I did not run `make format-check`, `make test`, firmware build/lint, or frontend checks because this was an audit-only request.
