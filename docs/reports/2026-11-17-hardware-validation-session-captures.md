# Hardware Validation Session Captures (2026-11-17)

**Date:** 2026-11-17  
**Source:** User-provided serial console captures (chat session)  
**Scope:** Consolidated validation script Sections A, B, D (partial). Section C and manual/reboot/interactive follow-ups not captured except where noted.  
**Status:** Partial evidence only — do not treat missing or corrupted steps as executed/passed.

## Device / environment (from captured output)
- **Boot/profile:** `Stored: default (none/invalid -> default)`, `Effective: default`, `Next boot: default`
- **Paths:** `/sdcard/scripts`, `/flash/scripts`
- **Network (examples):** Wi-Fi AP+STA connected to `<REDACTED_SSID>` (e.g. `192.168.0.187`, signal reported `-70 dBm` / `-67 dBm` / `-64 dBm` across snapshots), AP `192.168.4.1`
- **Storage (Section A):** Flash total 2031616 / used 81920 bytes; SD total 503054336 / used 999424 bytes
- **Firmware commit/build image:** not explicitly recorded in captures (record separately)
- **Note:** Memory/GC values changed across captures (Section A late: lower free/higher GC; Section B/D show higher free in some snapshots). A reboot/new session may have occurred between Section A and B — **not explicitly recorded**.

## Section A (consolidated automated pass, partial/corrupted)

### Usable baseline/meta evidence
- `services`: `http_server running`, `websocket running clients=0`, `wifi connected connected=yes stored_credentials=yes`, `bluetooth unknown enabled=no enabling=no connected=no`, `network idle diagnostics_worker=no`, `time_sync ready synced=yes`
- `berry app`: `none`
- `mem`/`berry profile`/`svc_mem`: coherent (GC initialized, HTTP worker not started / no HWM sample)
- `uptime`: `Up 11s`
- `info`: Flash and SD usage reported
- **R22 M3 (Item 1, partial):** `light` executed 3 times → `Light sensor: 0`, `0`, `0` (no lighting variation captured; explicit path/API/error cases not cleanly captured due to paste corruption)
- **R22 M4 (Item 2, partial):** `time foo` → full status `Local: 2026-11-17T17:19:48+01:00`, `UTC: 2026-11-17T16:19:48Z`, `Synced: yes` (unknown-arg fallthrough observed). Clean `time`, `time utc`, `time epoch`, and `system.time_info()` outputs not reliably captured (paste corruption / mangled evals)
- **R12 (Item 3, partial):** `ping 8.8.8.8` reply 36 ms, `ping 127.0.0.1` reply 1 ms, `nslookup example.com` → `172.66.147.243` (connected network). Touch scrollback not tested
- **Item 7A (partial-positive):** `svc_mem` repeated reads stable in captured window (identical values in two consecutive reads), fields/units/states/`n/a`/limitations shown
- **Item 10A (idle, partial-positive):** `services`, `berry app none`, `berry profile`, `mem` consistent
- **Items 4/5/6/8/9/11/12:** incomplete due to paste corruption, missing manual steps, or not executed

### Raw capture (as provided, includes corruption/mangled lines)
```text
COS/flash> berry eval "print('===== HW VALIDATION SESSION: CONSOLIDATED SCRIPT =====')"
===== HW VALIDATION SESSION: CONSOLIDATED SCRIPT =====
COS/flash> berry eval "print('Record manually: host date/time, board, firmware commit/build, storage mounted, Wi-Fi/BT env, notes')"
Record manually: host date/time, board, firmware commit/build, storage mounted, Wi-Fi/BT env, notes
COS/flash> 
COS/flash> berry eval "print('===== SECTION A: META / BASELINE (read-only) =====')"
===== SECTION A: META / BASELINE (read-only) =====
COS/flash> boot_profile
Stored:    default (none/invalid -> default)
Effective: default (current boot)
Next boot: default
Profiles:  default, low-memory-offline
Note: changes take effect on next boot/restart.
COS/flash> path list
/sdcard/scripts
/flash/scripts
COS/flash> services
[http_server] HTTP server: state=running
[websocket] WebSocket: state=running clients=0 http_server=yes
[wifi] Wi-Fi: state=connected connected=yes stored_credentials=yes
[bluetooth] Bluetooth: state=unknown enabled=no enabling=no connected=no
[network] Network: state=idle diagnostics_worker=no
[time_sync] Time sync: state=ready synced=yes
COS/flash> berry app
none
COS/flash> mem
Free heap:      67620 bytes
Min free heap:  62848 bytes
Total heap:     198200 bytes
Max alloc:      63488 bytes
Internal free:  67620 bytes
Internal min:   62848 bytes
Internal total: 198200 bytes
Internal max:   63488 bytes
COS/flash> berry profile
Berry memory profile (R2/R5/R8)
  Default heap:  free=66936 min=62848 total=198200 largest=63488
  Internal heap: free=66936 min=62848 total=198200 largest=63488
  Berry GC:      initialized=yes managed=5067 bytes step_rate=200
  Heap snapshot: free=66936 min=62848 largest=63488 internal_free=93012 internal_min=88892 internal_largest=63488
  HTTP worker:   not started (cfg=6144 unit=1)
COS/flash> svc_mem
svc_mem (service memory/budget snapshot, on-demand)
Units: bytes unless noted. n/a/unknown/unsupported values are reported explicitly.

[system_heap] System heap
  default_free: 66936 bytes
  default_min_free: 62848 bytes
  default_largest_block: 63488 bytes
  internal_free: 66936 bytes
  internal_min_free: 62848 bytes
  internal_largest_block: 63488 bytes

[berry_runtime] Berry runtime (state=running)
  initialized: true
  gc_managed: 5067 bytes
  gc_step_rate: 200
  heap_free: 66936 bytes
  heap_min_free: 62848 bytes
  heap_largest_block: 63488 bytes
  internal_free: 93012 bytes
  internal_min_free: 88892 bytes
  internal_largest_block: 63488 bytes

[network_http] Network/HTTP worker (state=idle)
  worker_stack_cfg: 6144 bytes
  worker_stack_hwm: n/a (no sample yet)
  worker_stack_unit: 1 bytes/unit
  worker_hwm_available: false
  notes: bounded body/buffer limits per network.h (e.g. NETWORK_HTTP_BODY_MAX=2048)

[wifi] Wi-Fi (state=connected)
  metrics: n/a (no cheap per-subsystem heap counters exposed; ESP-IDF reclaimability limited)

[bluetooth] Bluetooth (state=unknown)
  metrics: n/a (no cheap scan/buffer/task counters exposed)

[text_console] Text console (state=unknown)
  cols: unknown (not exposed)
  rows: unknown (not exposed)
  font_size: unknown (not exposed)
  notes: task stack/HWM and buffer capacities not exposed as cheap accessors; semantics follow text_console lifecycle plan

[shell_input] Shell input (state=ready)
  metrics: n/a (stack/HWM and input buffer/queue sizes not exposed as cheap accessors)

Limitations: on-demand snapshot only; point-in-time values; ESP-IDF resources may be non-reclaimable.
COS/flash> uptime
Up 11s
COS/flash> info
Flash (LittleFS):
  Total: 2031616 bytes
  Used:  81920 bytes
SD card (SD):
  Total: 503054336 bytes
  Used:  999424 bytes
COS/flash> berry eval "print('--- META END ---')"
--- META END ---
COS/flash> 
COS/flash> berry eval "print('===== ITEM 1: R22 M3 light =====')"
===== ITEM 1: R22 M3 light =====
COS/flash> berry eval "print('--- 1A: basic (repeat manually under different lighting and record values 0..4095) ---')"
--- 1A: basic (repeat manually under different lighting and record values 0..4095) ---
COS/flash> light
Light sensor: 0
COS/flash> light
Light sensor: 0
COS/flash> light
Light sensor: 0
COS/flash> berry eval "print('--- 1B: expli_import('@cos/light'); var l = m.find('default'); print(l.read())"
E (13282) berry: shell: shell:1: stray '\64' in program
berry: execution failed (ESP_FAIL)
COS/flash> berry eval "print('--- 1D: errortime epoch
E (13292) berry: shell: shell:1: unfinished string
berry: execution failed (ESP_FAIL)
COS/flash> time foo
Local:  2026-11-17T17:19:48+01:00
UTC:    2026-11-17T16:19:48Z
Synced: yes
COS/flash> berry eval "print('--- 2B: system.time_info() API + resolver notes ---')"
--- 2B: system.time_info() API + resolver notes ---
COS/flash> berry eval "var m = cos_import('@cos/sative/unaffected; record exact outputs/sync state/env/commit for evidence.')"
E (13362) berry: shell: module not found: @cos/sative/unaffected; record exact outputs/sync state/env/commit for evidence.
berry: execution failed (ESP_FAIL)
COS/flash> 
COS/flash> berry eval "print('===== ITEM 3: R12 Milestone 4 (data collection; acceptance may need cla ---'ation) =====')"
E (13392) berry: shell: shell:1: expected ')' before 'ation'
berry: execution failed (ESP_FAIL)
COS/flash> berry eval "print('--- 3A: current network command behavior (requires Wi-Fi/network; capture actual output) ---')"
--- 3A: current network command behavior (requires Wi-Fi/network; capture actual output) ---
COS/flash> wifi status
Service: started
User enabled (persisted): yes
Boot profile: allowed
Effective: enabled
Mode:    AP+STA
AP IP:   192.168.4.1
STA:     <REDACTED_SSID>
STA IP:  192.168.0.187
Signal:  -70 dBm (Not good)
MAC:     34:5F:45:AA:CF:40
COS/flash> services
[http_server] HTTP server: state=running
[websocket] WebSocket: state=running clients=0 http_server=yes
[wifi] Wi-Fi: state=connected connected=yes stored_credentials=yes
[bluetooth] Bluetooth: state=unknown enabled=no enabling=no connected=no
[network] Network: state=idle diagnostics_worker=no
[time_sync] Time sync: state=ready synced=yes
COS/flash> ping 8.8.8.8
Reply from 8.8.8.8: time=36 ms
COS/flash> ping 127.0.0.1
Reply from 127.0.0.1: time=1 ms
COS/flash> nslookup example.com
Name: example.com
Address: 172.66.147.243
COS/flash> berry eval "print('--- 3B: touch scrollback gestures = MANUAL ---')"
--- 3B: touch scrollback gestures = MANUAL ---
COS/flash> berry eval "print('MANUAL: perform Milestone 4 touch scrollback gestures on device; record pass/fail/behavior/screenshots-notes
E (13682) berry: shell: shell:1: unfinished string
berry: execution failed (ESP_FAIL)
COS/flash> berry eval "pr
E (13702) berry: shell: shell:1: 'pr' undeclared (first use in this function)
berry: execution failed (ESP_FAIL)
COS/flash> 
COS/flash> berry eval "print('===== ITEM 4: Unified service lifecycle smoke (post-boot/default behavior snapshot) =====')"
===== ITEM 4: Unified service lifecycle smoke (post-boot/default behavior snapshot) =====
COS/flash> berry eval "prmem
E (13732) berry: shell: shell:1: 'prmem' undeclared (first use in this function)
berry: execution failed (ESP_FAIL)
COS/flash> berry profile
Berry memory profile (R2/R5/R8)
  Default heap:  free=45064 min=40648 total=198200 largest=38912
  Internal heap: free=45064 min=40648 total=198200 largest=38912
  Berry GC:      initialized=yes managed=10390 bytes step_rate=200
  Heap snapshot: free=45064 min=40648 largest=38912 internal_free=71140 internal_min=66692 internal_largest=38912
  HTTP worker:   not started (cfg=6144 unit=1)
COS/flash> svc_mem
svc_mem (service memory/budget snapshot, on-demand)
Units: bytes unless noted. n/a/unknown/unsupported values are reported explicitly.

[system_heap] System heap
  default_free: 45064 bytes
  default_min_free: 40648 bytes
  default_largest_block: 38912 bytes
  internal_free: 45064 bytes
  internal_min_free: 40648 bytes
  internal_largest_block: 38912 bytes

[berry_runtime] Berry runtime (state=running)
  initialized: true
  gc_managed: 10390 bytes
  gc_step_rate: 200
  heap_free: 45064 bytes
  heap_min_free: 40648 bytes
  heap_largest_block: 38912 bytes
  internal_free: 71140 bytes
  internal_min_free: 66692 bytes
  internal_largest_block: 38912 bytes

[network_http] Network/HTTP worker (state=idle)
  worker_stack_cfg: 6144 bytes
  worker_stack_hwm: n/a (no sample yet)
  worker_stack_unit: 1 bytes/unit
  worker_hwm_available: false
  notes: bounded body/buffer limits per network.h (e.g. NETWORK_HTTP_BODY_MAX=2048)

[wifi] Wi-Fi (state=connected)
  metrics: n/a (no cheap per-subsystem heap counters exposed; ESP-IDF reclaimability limited)

[bluetooth] Bluetooth (state=unknown)
  metrics: n/a (no cheap scan/buffer/task counters exposed)

[text_console] Text console (state=unknown)
  cols: unknown (not exposed)
  rows: unknown (not exposed)
  font_size: unknown (not exposed)
  notes: task stack/HWM and buffer capacities not exposed as cheap accessors; semantics follow text_console lifecycle plan

[shell_input] Shell input (state=ready)
  metrics: n/a (stack/HWM and input buffer/queue sizes not exposed as cheap accessors)

Limitations: on-demand snapshot only; point-in-time values; ESP-IDF resources may be non-reclaimable.
COS/flash> 
COS/flash> berry eval "print('===== ITEM 5: Boot profiles & feature modes (read-only snapshot + MANUAL/REBOOapp
E (13942) berry: shell: shell:1: unfinished string
berry: execution failed (ESP_FAIL)
COS/flash> mem
Free heap:      43520 bytes
Min free heap:  40648 bytes
Total heap:     198200 bytes
Max alloc:      40960 bytes
Internal free:  43520 bytes
Internal min:   40648 bytes
Internal total: 198200 bytes
Internal max:   40960 bytes
COS/flash> berry profile
Berry memory profile (R2/R5/R8)
  Default heap:  free=42888 min=40648 total=198200 largest=40960
  Internal heap: free=42888 min=40648 total=198200 largest=40960
  Berry GC:      initialized=yes managed=13592 bytes step_rate=200
  Heap snapshot: free=42888 min=40648 largest=40960 internal_free=68964 internal_min=66692 internal_largest=40960
  HTTP worker:   not started (cfg=6144 unit=1)
COS/flash> svc_mem
svc_mem (service memory/budget snapshot, on-demand)
Units: bytes unless noted. n/a/unknown/unsupported values are reported explicitly.

[system_heap] System heap
  default_free: 42888 bytes
  default_min_free: 40648 bytes
  default_largest_block: 40960 bytes
  internal_free: 42888 bytes
  internal_min_free: 40648 bytes
  internal_largest_block: 40960 bytes

[berry_runtime] Berry runtime (state=running)
  initialized: true
  gc_managed: 13592 bytes
  gc_step_rate: 200
  heap_free: 42888 bytes
  heap_min_free: 40648 bytes
  heap_largest_block: 40960 bytes
  internal_free: 68964 bytes
  internal_min_free: 66692 bytes
  internal_largest_block: 40960 bytes

[network_http] Network/HTTP worker (state=idle)
  worker_stack_cfg: 6144 bytes
  worker_stack_hwm: n/a (no sample yet)
  worker_stack_unit: 1 bytes/unit
  worker_hwm_available: false
  notes: bounded body/buffer limits per network.h (e.g. NETWORK_HTTP_BODY_MAX=2048)

[wifi] Wi-Fi (state=connected)
  metrics: n/a (no cheap per-subsystem heap counters exposed; ESP-IDF reclaimability limited)

[bluetooth] Bluetooth (state=unknown)
  metrics: n/a (no cheap scan/buffer/task counters exposed)

[text_console] Text console (state=unknown)
  cols: unknown (not exposed)
  rows: unknown (not exposed)
  font_size: unknown (not exposed)
  notes: task stack/HWM and buffer capacities not exposed as cheap accessors; semantics follow text_console lifecycle plan

[shell_input] Shell input (state=ready)
  metrics: n/a (stack/HWM and input buffer/queue sizes not exposed as cheap accessors)

Limitations: on-demand snapshot only; point-in-time values; ESP-IDF resources may be non-reclaimable.
COS/flash> 
COS/flash> berry eval "print('===== ITEM 5: Boot profiles & feature modes (read-only snapshot + MANUAL/REBOOT required) =====')"
===== ITEM 5: Boot profiles & feature modes (read-only snapshot + MANUAL/REBOOT required) =====
COS/flash> berry eval "print('--- 5A: current profile baseline ---')"
--- 5A: current profile baseline ---
COS/flash> b--')"
Unknown command: b--')"
COS/flash> 6: Text console memory & lifecycle (snapshot + MANUAL transitions required) =====')"
Unknown command: 6: Text console memory & lifecycle (snapshot + MANUAL transitions required) =====')"
COS/flash> berry eval "print('--- 6A: baseline ---')"
--- 6A: baseline ---
COS/flash> ')"
Unknown command: ')"
COS/flash> berry eval "print('--- 7A: baseline and repent('===== ITEM 7: Service memory budget observability (baseline/stability) =====')"
E (14262) berry: shell: shell:1: unexpected symbol near '=='
berry: execution failed (ESP_FAIL)
COS/flash> berry eval "print('--- 7A: baseline and repeated reads ---')"
--- 7A: baseline and repeated reads ---
COS/flash> svc_mem
svc_mem (service memory/budget snapshot, on-demand)
Units: bytes unless noted. n/a/unknown/unsupported values are reported explicitly.

[system_heap] System heap
  default_free: 42180 bytes
  default_min_free: 39300 bytes
  default_largest_block: 40960 bytes
  internal_free: 42180 bytes
  internal_min_free: 39300 bytes
  internal_largest_block: 40960 bytes

[berry_runtime] Berry runtime (state=running)
  initialized: true
  gc_managed: 15280 bytes
  gc_step_rate: 200
  heap_free: 42180 bytes
  heap_min_free: 39300 bytes
  heap_largest_block: 40960 bytes
  internal_free: 68256 bytes
  internal_min_free: 65344 bytes
  internal_largest_block: 40960 bytes

[network_http] Network/HTTP worker (state=idle)
  worker_stack_cfg: 6144 bytes
  worker_stack_hwm: n/a (no sample yet)
  worker_stack_unit: 1 bytes/unit
  worker_hwm_available: false
  notes: bounded body/buffer limits per network.h (e.g. NETWORK_HTTP_BODY_MAX=2048)

[wifi] Wi-Fi (state=connected)
  metrics: n/a (no cheap per-subsystem heap counters exposed; ESP-IDF reclaimability limited)

[bluetooth] Bluetooth (state=unknown)
  metrics: n/a (no cheap scan/buffer/task counters exposed)

[text_console] Text console (state=unknown)
  cols: unknown (not exposed)
  rows: unknown (not exposed)
  font_size: unknown (not exposed)
  notes: task stack/HWM and buffer capacities not exposed as cheap accessors; semantics follow text_console lifecycle plan

[shell_input] Shell input (state=ready)
  metrics: n/a (stack/HWM and input buffer/queue sizes not exposed as cheap accessors)

Limitations: on-demand snapshot only; point-in-time values; ESP-IDF resources may be non-reclaimable.
COS/flash> svc_mem
svc_mem (service memory/budget snapshot, on-demand)
Units: bytes unless noted. n/a/unknown/unsupported values are reported explicitly.

[system_heap] System heap
  default_free: 42180 bytes
  default_min_free: 39300 bytes
  default_largest_block: 40960 bytes
  internal_free: 42180 bytes
  internal_min_free: 39300 bytes
  internal_largest_block: 40960 bytes

[berry_runtime] Berry runtime (state=running)
  initialized: true
  gc_managed: 15280 bytes
  gc_step_rate: 200
  heap_free: 42180 bytes
  heap_min_free: 39300 bytes
  heap_largest_block: 40960 bytes
  internal_free: 68256 bytes
  internal_min_free: 65344 bytes
  internal_largest_block: 40960 bytes

[network_http] Network/HTTP worker (state=idle)
  worker_stack_cfg: 6144 bytes
  worker_stack_hwm: n/a (no sample yet)
  worker_stack_unit: 1 bytes/unit
  worker_hwm_available: false
  notes: bounded body/buffer limits per network.h (e.g. NETWORK_HTTP_BODY_MAX=2048)

[wifi] Wi-Fi (state=connected)
  metrics: n/a (no cheap per-subsystem heap counters exposed; ESP-IDF reclaimability limited)

[bluetooth] Bluetooth (state=unknown)
  metrics: n/a (no cheap scan/buffer/task counters exposed)

[text_console] Text console (state=unknown)
  cols: unknown (not exposed)
  rows: unknown (not exposed)
  font_size: unknown (not exposed)
  notes: task stack/HWM and buffer capacities not exposed as cheap accessors; semantics follow text_console lifecycle plan

[shell_input] Shell input (state=ready)
  metrics: n/a (stack/HWM and input buffer/queue sizes not exposed as cheap accessors)

Limitations: on-demand snapshot only; point-in-time values; ESP-IDF resources may be non-reclaimable.
COS/flash> mem
Free heap:      47168 bytes
Min free heap:  39300 bytes
Total heap:     198200 bytes
Max alloc:      40960 bytes
Internal free:  47168 bytes
Internal min:   39300 bytes
Internal total: 198200 bytes
Internal max:   40960 bytes
COS/flash> berry profile
Berry memory profile (R2/R5/R8)
  Default heap:  free=46720 min=39300 total=198200 largest=40960
  Internal heap: free=46720 min=39300 total=198200 largest=40960
  Berry GC:      initialized=yes managed=17792 bytes step_rate=200
  Heap snapshot: free=46720 min=39300 largest=40960 internal_free=72796 internal_min=65344 internal_largest=40960
  HTTP worker:   not started (cfg=6144 unit=1)
COS/flash> services
[http_server] HTTP server: state=running
[websocket] WebSocket: state=running clients=0 http_server=yes
[wifi] Wi-Fi: state=connected connected=yes stored_credentials=yes
[bluetooth] Bluetooth: state=unknown enabled=no enabling=no connected=no
[network] Network: state=idle diagnostics_worker=no
[time_sync] Time sync: state=ready synced=yes
COS/flash> berry eval "print('MANUAL 7B/7C: perform controllable A-B start/stop cycle(s) and R2-style observations; record device-measured
E (14712) berry: shell: shell:1: unfinished string
berry: execution failed (ESP_FAIL)
COS/flash> 
COS/flash> berry eval "print('===== ITtdin) ---')"
===== ITtdin) ---
COS/flash> berry eval "print('PIPE_HELLO')" | /flash/scripts/pipe_stdin.be
COS/flash> time e
Local:  2026-11-17T17:19:50+01:00
UTC:    2026-11-17T16:19:50Z
Synced: yes
COS/flash> 
COS/flash> berry eval "print('===== ITEM 10: Berry ru) print('01234567890123456789012345678'); i=i+1; end" | /flash/scripts/pipe_stdin.be
COS/flash> 
COS/flash> berry eval "print('===== ITEM 10: Berry runtime & service inspection v1 =====')"
===== ITEM 10: Berry runtime & service inspection v1 =====
COS/flash> berry eval "print('--- 10A: idle inspection ---')"
--- 10A: idle inspection ---
COS/flash> services
[http_server] HTTP server: state=running
[websocket] WebSocket: state=running clients=0 http_server=yes
[wifi] Wi-Fi: state=connected connected=yes stored_credentials=yes
[bluetooth] Bluetooth: state=unknown enabled=no enabling=no connected=no
[network] Network: state=idle diagnostics_worker=no
[time_sync] Time sync: state=ready synced=yes
COS/flash> berry app
none
COS/flash> berry profile
Berry memory profile (R2/R5/R8)
  Default heap:  free=48096 min=39300 total=198200 largest=40960
  Internal heap: free=48096 min=39300 total=198200 largest=40960
  Berry GC:      initialized=yes managed=15763 bytes step_rate=200
  Heap snapshot: free=48096 min=39300 largest=40960 internal_free=74172 internal_min=65344 internal_largest=40960
  HTTP worker:   not started (cfg=6144 unit=1)
COS/flash> mem
Free heap:      46160 bytes
Min free heap:  39300 bytes
Total heap:     198200 bytes
Max alloc:      40960 bytes
Internal free:  46160 bytes
Internal min:   39300 bytes
Internal total: 198200 bytes
Internal max:   40960 bytes
COS/flash> berry eval "print('MANUAL 10B: launch a Bem
E (15152) berry: shell: shell:1: unfinished string
berry: execution failed (ESP_FAIL)
COS/flash> svc_mem
svc_mem (service memory/budget snapshot, on-demand)
Units: bytes unless noted. n/a/unknown/unsupported values are reported explicitly.

[system_heap] System heap
  default_free: 50752 bytes
  default_min_free: 39300 bytes
  default_largest_block: 40960 bytes
  internal_free: 50752 bytes
  internal_min_free: 39300 bytes
  internal_largest_block: 40960 bytes

[berry_runtime] Berry runtime (state=running)
  initialized: true
  gc_managed: 10623 bytes
  gc_step_rate: 200
  heap_free: 50752 bytes
  heap_min_free: 39300 bytes
  heap_largest_block: 40960 bytes
  internal_free: 76828 bytes
  internal_min_free: 65344 bytes
  internal_largest_block: 40960 bytes

[network_http] Network/HTTP worker (state=idle)
  worker_stack_cfg: 6144 bytes
  worker_stack_hwm: n/a (no sample yet)
  worker_stack_unit: 1 bytes/unit
  worker_hwm_available: false
  notes: bounded body/buffer limits per network.h (e.g. NETWORK_HTTP_BODY_MAX=2048)

[wifi] Wi-Fi (state=connected)
  metrics: n/a (no cheap per-subsystem heap counters exposed; ESP-IDF reclaimability limited)

[bluetooth] Bluetooth (state=unknown)
  metrics: n/a (no cheap scan/buffer/task counters exposed)

[text_console] Text console (state=unknown)
  cols: unknown (not exposed)
  rows: unknown (not exposed)
  font_size: unknown (not exposed)
  notes: task stack/HWM and buffer capacities not exposed as cheap accessors; semantics follow text_console lifecycle plan

[shell_input] Shell input (state=ready)
  metrics: n/a (stack/HWM and input buffer/queue sizes not exposed as cheap accessors)

Limitations: on-demand snapshot only; point-in-time values; ESP-IDF resources may be non-reclaimable.
COS/flash> berry eval "pstart/stop (baseline + OPTIONAL state changes in SECTION D) =====')"
E (15312) berry: shell: shell:1: 'pstart' undeclared (first use in this function)
berry: execution failed (ESP_FAIL)
COS/flash> boot_profile
Stored:    default (none/invalid -> default)
Effective: default (current boot)
Next boot: default
Profiles:  default, low-memory-offline
Note: changes take effect on next boot/restart.
COS/flash> wifi status
Service: started
User enabled (persisted): yes
Boot profile: allowed
Effective: enabled
Mode:    AP+STA
AP IP:   192.168.4.1
STA:     <REDACTED_SSID>
STA IP:  192.168.0.187
Signal:  -71 dBm (Not good)
MAC:     34:5F:45:AA:CF:40
COS/flash> services
[http_server] HTTP server: state=running
[websocket] WebSocket: state=running clients=0 http_server=yes
[wifi] Wi-Fi: state=connected connected=yes stored_credentials=yes
[bluetooth] Bluetooth: state=unknown enabled=no enabling=no connected=no
[network] Network: state=idle diagnostics_worker=no
[time_sync] Time sync: state=ready synced=yes
COS/flash> mem
Free heap:      49072 bytes
Min free heap:  39300 bytes
Total heap:     198200 bytes
Max alloc:      40960 bytes
Internal free:  49072 bytes
Internal min:   39300 bytes
Internal total: 198200 bytes
Internal max:   40960 bytes
COS/flash> svc_mem
svc_mem (service memory/budget snapshot, on-demand)
Units: bytes unless noted. n/a/unknown/unsupported values are reported explicitly.

[system_heap] System heap
  default_free: 48476 bytes
  default_min_free: 39300 bytes
  default_largest_block: 40960 bytes
  internal_free: 48476 bytes
  internal_min_free: 39300 bytes
  internal_largest_block: 40960 bytes

[berry_runtime] Berry runtime (state=running)
  initialized: true
  gc_managed: 13898 bytes
  gc_step_rate: 200
  heap_free: 48476 bytes
  heap_min_free: 39300 bytes
  heap_largest_block: 40960 bytes
  internal_free: 74552 bytes
  internal_min_free: 65344 bytes
  internal_largest_block: 40960 bytes

[network_http] Network/HTTP worker (state=idle)
  worker_stack_cfg: 6144 bytes
  worker_stack_hwm: n/a (no sample yet)
  worker_stack_unit: 1 bytes/unit
  worker_hwm_available: false
  notes: bounded body/buffer limits per network.h (e.g. NETWORK_HTTP_BODY_MAX=2048)

[wifi] Wi-Fi (state=connected)
  metrics: n/a (no cheap per-subsystem heap counters exposed; ESP-IDF reclaimability limited)

[bluetooth] Bluetooth (state=unknown)
  metrics: n/a (no cheap scan/buffer/task counters exposed)

[text_console] Text console (state=unknown)
  cols: unknown (not exposed)
  rows: unknown (not exposed)
  font_size: unknown (not exposed)
  notes: task stack/HWM and buffer capacities not exposed as cheap accessors; semantics follow text_console lifecycle plan

[shell_input] Shell input (state=ready)
  metrics: n/a (stack/HWM and input buffer/queue sizes not exposed as cheap accessors)

Limitations: on-demand snapshot only; point-in-time values; ESP-IDF resources may be non-reclaimable.
COS/flash> berry eval " output.')"ual/reboot follow-ups (SECTION B/C) and optional state-changing checks (SECTION D) as needed, then send 
E (19992) berry: shell: shell:1: 'output' undeclared (first use in this function)
berry: execution failed (ESP_FAIL)
COS/flash> 
```

## Section B (snapshot / post-state check, clean)

### Raw capture
```text
COS/flash> berry eval "print('===== SECTION B: SNAPSHOT / POST-STATE CHECK =====')"
===== SECTION B: SNAPSHOT / POST-STATE CHECK =====
COS/flash> boot_profile
Stored:    default (none/invalid -> default)
Effective: default (current boot)
Next boot: default
Profiles:  default, low-memory-offline
Note: changes take effect on next boot/restart.
COS/flash> services
[http_server] HTTP server: state=running
[websocket] WebSocket: state=running clients=0 http_server=yes
[wifi] Wi-Fi: state=connected connected=yes stored_credentials=yes
[bluetooth] Bluetooth: state=unknown enabled=no enabling=no connected=no
[network] Network: state=idle diagnostics_worker=no
[time_sync] Time sync: state=ready synced=yes
COS/flash> wifi status
Service: started
User enabled (persisted): yes
Boot profile: allowed
Effective: enabled
Mode:    AP+STA
AP IP:   192.168.4.1
STA:     <REDACTED_SSID>
STA IP:  192.168.0.187
Signal:  -67 dBm (Okay)
MAC:     34:5F:45:AA:CF:40
COS/flash> bt status
Usage: bt [on | off | scan | connect <addr> | disconnect | forget]
COS/flash> berry app
none
COS/flash> mem
Free heap:      66676 bytes
Min free heap:  64980 bytes
Total heap:     198200 bytes
Max alloc:      63488 bytes
Internal free:  66676 bytes
Internal min:   64980 bytes
Internal total: 198200 bytes
Internal max:   63488 bytes
COS/flash> berry profile
Berry memory profile (R2/R5/R8)
  Default heap:  free=65384 min=64980 total=198200 largest=63488
  Internal heap: free=65384 min=64980 total=198200 largest=63488
  Berry GC:      initialized=yes managed=6144 bytes step_rate=200
  Heap snapshot: free=65384 min=64980 largest=63488 internal_free=91460 internal_min=91024 internal_largest=63488
  HTTP worker:   not started (cfg=6144 unit=1)
COS/flash> svc_mem
svc_mem (service memory/budget snapshot, on-demand)
Units: bytes unless noted. n/a/unknown/unsupported values are reported explicitly.

[system_heap] System heap
  default_free: 65508 bytes
  default_min_free: 61988 bytes
  default_largest_block: 63488 bytes
  internal_free: 65508 bytes
  internal_min_free: 61988 bytes
  internal_largest_block: 63488 bytes

[berry_runtime] Berry runtime (state=running)
  initialized: true
  gc_managed: 6144 bytes
  gc_step_rate: 200
  heap_free: 65508 bytes
  heap_min_free: 61988 bytes
  heap_largest_block: 63488 bytes
  internal_free: 91584 bytes
  internal_min_free: 88032 bytes
  internal_largest_block: 63488 bytes

[network_http] Network/HTTP worker (state=idle)
  worker_stack_cfg: 6144 bytes
  worker_stack_hwm: n/a (no sample yet)
  worker_stack_unit: 1 bytes/unit
  worker_hwm_available: false
  notes: bounded body/buffer limits per network.h (e.g. NETWORK_HTTP_BODY_MAX=2048)

[wifi] Wi-Fi (state=connected)
  metrics: n/a (no cheap per-subsystem heap counters exposed; ESP-IDF reclaimability limited)

[bluetooth] Bluetooth (state=unknown)
  metrics: n/a (no cheap scan/buffer/task counters exposed)

[text_console] Text console (state=unknown)
  cols: unknown (not exposed)
  rows: unknown (not exposed)
  font_size: unknown (not exposed)
  notes: task stack/HWM and buffer capacities not exposed as cheap accessors; semantics follow text_console lifecycle plan

[shell_input] Shell input (state=ready)
  metrics: n/a (stack/HWM and input buffer/queue sizes not exposed as cheap accessors)

Limitations: on-demand snapshot only; point-in-time values; ESP-IDF resources may be non-reclaimable.
```

### Observations
- Clean snapshot in default profile: services/state reporting coherent, Wi-Fi connected, `bt status` prints usage (subcommand form differs from `status`), Berry idle, healthy heap/GC values.
- Higher free heap/lower GC than late Section A; reboot/new-session boundary not explicitly recorded.

## Section D (optional Wi-Fi A-B / lifecycle, clean)

### Raw capture
```text
COS/flash> berry eval "print('===== SECTION D: OPTIONAL WIFI A-B / LIFECYCLE =====')"
===== SECTION D: OPTIONAL WIFI A-B / LIFECYCLE =====
COS/flash> wifi status
Service: started
User enabled (persisted): yes
Boot profile: allowed
Effective: enabled
Mode:    AP+STA
AP IP:   192.168.4.1
STA:     <REDACTED_SSID>
STA IP:  192.168.0.187
Signal:  -64 dBm (Very good)
MAC:     34:5F:45:AA:CF:40
COS/flash> svc_mem
svc_mem (service memory/budget snapshot, on-demand)
Units: bytes unless noted. n/a/unknown/unsupported values are reported explicitly.

[system_heap] System heap
  default_free: 69028 bytes
  default_min_free: 67888 bytes
  default_largest_block: 65536 bytes
  internal_free: 69028 bytes
  internal_min_free: 67888 bytes
  internal_largest_block: 65536 bytes

[berry_runtime] Berry runtime (state=running)
  initialized: true
  gc_managed: 2710 bytes
  gc_step_rate: 200
  heap_free: 69028 bytes
  heap_min_free: 67888 bytes
  heap_largest_block: 65536 bytes
  internal_free: 95104 bytes
  internal_min_free: 93932 bytes
  internal_largest_block: 65536 bytes

[network_http] Network/HTTP worker (state=idle)
  worker_stack_cfg: 6144 bytes
  worker_stack_hwm: n/a (no sample yet)
  worker_stack_unit: 1 bytes/unit
  worker_hwm_available: false
  notes: bounded body/buffer limits per network.h (e.g. NETWORK_HTTP_BODY_MAX=2048)

[wifi] Wi-Fi (state=connected)
  metrics: n/a (no cheap per-subsystem heap counters exposed; ESP-IDF reclaimability limited)

[bluetooth] Bluetooth (state=unknown)
  metrics: n/a (no cheap scan/buffer/task counters exposed)

[text_console] Text console (state=unknown)
  cols: unknown (not exposed)
  rows: unknown (not exposed)
  font_size: unknown (not exposed)
  notes: task stack/HWM and buffer capacities not exposed as cheap accessors; semantics follow text_console lifecycle plan

[shell_input] Shell input (state=ready)
  metrics: n/a (stack/HWM and input buffer/queue sizes not exposed as cheap accessors)

Limitations: on-demand snapshot only; point-in-time values; ESP-IDF resources may be non-reclaimable.
COS/flash> mem
Free heap:      66548 bytes
Min free heap:  65140 bytes
Total heap:     198200 bytes
Max alloc:      63488 bytes
Internal free:  66548 bytes
Internal min:   65140 bytes
Internal total: 198200 bytes
Internal max:   63488 bytes
COS/flash> wifi stop
Wi-Fi stopped (disabled state persisted)
COS/flash> berry eval "print('--- after wifi stop ---')"
--- after wifi stop ---
COS/flash> wifi status
Service: stopped
User enabled (persisted): no
Boot profile: allowed
Effective: user disabled
Mode:    OFF
STA:     (service stopped)
AP:      (service stopped)
COS/flash> services
[http_server] HTTP server: state=running
[websocket] WebSocket: state=running clients=0 http_server=yes
[wifi] Wi-Fi: state=stopped connected=no stored_credentials=yes
[bluetooth] Bluetooth: state=unknown enabled=no enabling=no connected=no
[network] Network: state=idle diagnostics_worker=no
[time_sync] Time sync: state=ready synced=yes
COS/flash> svc_mem
svc_mem (service memory/budget snapshot, on-demand)
Units: bytes unless noted. n/a/unknown/unsupported values are reported explicitly.

[system_heap] System heap
  default_free: 75644 bytes
  default_min_free: 64980 bytes
  default_largest_block: 63488 bytes
  internal_free: 75644 bytes
  internal_min_free: 64980 bytes
  internal_largest_block: 63488 bytes

[berry_runtime] Berry runtime (state=running)
  initialized: true
  gc_managed: 4239 bytes
  gc_step_rate: 200
  heap_free: 75644 bytes
  heap_min_free: 64980 bytes
  heap_largest_block: 63488 bytes
  internal_free: 101720 bytes
  internal_min_free: 91024 bytes
  internal_largest_block: 63488 bytes

[network_http] Network/HTTP worker (state=idle)
  worker_stack_cfg: 6144 bytes
  worker_stack_hwm: n/a (no sample yet)
  worker_stack_unit: 1 bytes/unit
  worker_hwm_available: false
  notes: bounded body/buffer limits per network.h (e.g. NETWORK_HTTP_BODY_MAX=2048)

[wifi] Wi-Fi (state=stopped)
  metrics: n/a (no cheap per-subsystem heap counters exposed; ESP-IDF reclaimability limited)

[bluetooth] Bluetooth (state=unknown)
  metrics: n/a (no cheap scan/buffer/task counters exposed)

[text_console] Text console (state=unknown)
  cols: unknown (not exposed)
  rows: unknown (not exposed)
  font_size: unknown (not exposed)
  notes: task stack/HWM and buffer capacities not exposed as cheap accessors; semantics follow text_console lifecycle plan

[shell_input] Shell input (state=ready)
  metrics: n/a (stack/HWM and input buffer/queue sizes not exposed as cheap accessors)

Limitations: on-demand snapshot only; point-in-time values; ESP-IDF resources may be non-reclaimable.
COS/flash> mem
Free heap:      74212 bytes
Min free heap:  64980 bytes
Total heap:     198200 bytes
Max alloc:      63488 bytes
Internal free:  74212 bytes
Internal min:   64980 bytes
Internal total: 198200 bytes
Internal max:   63488 bytes
COS/flash> wifi scan
wifi: scan failed: service stopped/disabled (use 'wifi start' in allowed profiles)
COS/flash> wifi restart
wifi: restart failed: service stopped/disabled (use 'wifi start' in allowed profiles)
COS/flash> berry eval "print('--- expect denied while stopped/forced-off as applicable ---')"
--- expect denied while stopped/forced-off as applicable ---
COS/flash> wifi start
Wi-Fi started (enabled state persisted)
COS/flash> berry eval "print('--- after wifi start ---')"
--- after wifi start ---
COS/flash> wifi status
Service: started
User enabled (persisted): yes
Boot profile: allowed
Effective: enabled
Mode:    AP+STA
AP IP:   192.168.4.1
STA:     <REDACTED_SSID>
STA IP:  (not connected)
MAC:     34:5F:45:AA:CF:40
COS/flash> services
[http_server] HTTP server: state=running
[websocket] WebSocket: state=running clients=0 http_server=yes
[wifi] Wi-Fi: state=running connected=no stored_credentials=yes
[bluetooth] Bluetooth: state=unknown enabled=no enabling=no connected=no
[network] Network: state=idle diagnostics_worker=no
[time_sync] Time sync: state=ready synced=yes
COS/flash> wifi restart
Wi-Fi service restarted
COS/flash> wifi status
Service: started
User enabled (persisted): yes
Boot profile: allowed
Effective: enabled
Mode:    AP+STA
AP IP:   192.168.4.1
STA:     <REDACTED_SSID>
STA IP:  (not connected)
MAC:     34:5F:45:AA:CF:40
COS/flash> svc_mem
svc_mem (service memory/budget snapshot, on-demand)
Units: bytes unless noted. n/a/unknown/unsupported values are reported explicitly.

[system_heap] System heap
  default_free: 64548 bytes
  default_min_free: 62376 bytes
  default_largest_block: 61440 bytes
  internal_free: 64548 bytes
  internal_min_free: 62376 bytes
  internal_largest_block: 61440 bytes

[berry_runtime] Berry runtime (state=running)
  initialized: true
  gc_managed: 7722 bytes
  gc_step_rate: 200
  heap_free: 64548 bytes
  heap_min_free: 62376 bytes
  heap_largest_block: 61440 bytes
  internal_free: 90624 bytes
  internal_min_free: 88420 bytes
  internal_largest_block: 61440 bytes

[network_http] Network/HTTP worker (state=idle)
  worker_stack_cfg: 6144 bytes
  worker_stack_hwm: n/a (no sample yet)
  worker_stack_unit: 1 bytes/unit
  worker_hwm_available: false
  notes: bounded body/buffer limits per network.h (e.g. NETWORK_HTTP_BODY_MAX=2048)

[wifi] Wi-Fi (state=running)
  metrics: n/a (no cheap per-subsystem heap counters exposed; ESP-IDF reclaimability limited)

[bluetooth] Bluetooth (state=unknown)
  metrics: n/a (no cheap scan/buffer/task counters exposed)

[text_console] Text console (state=unknown)
  cols: unknown (not exposed)
  rows: unknown (not exposed)
  font_size: unknown (not exposed)
  notes: task stack/HWM and buffer capacities not exposed as cheap accessors; semantics follow text_console lifecycle plan

[shell_input] Shell input (state=ready)
  metrics: n/a (stack/HWM and input buffer/queue sizes not exposed as cheap accessors)

Limitations: on-demand snapshot only; point-in-time values; ESP-IDF resources may be non-reclaimable.
COS/flash> mem
Free heap:      64088 bytes
Min free heap:  60036 bytes
Total heap:     198200 bytes
Max alloc:      57344 bytes
Internal free:  64088 bytes
Internal min:   60036 bytes
Internal total: 198200 bytes
Internal max:   57344 bytes
```

### Observations (partial-positive evidence)
- **Lifecycle/state transitions:** `wifi stop` persisted disabled and moved service to stopped (Mode/STA/AP off); `services` reflected `wifi stopped`. `wifi start` re-enabled/persisted and moved to running; active `wifi restart` preserved persisted enabled intent while STA had not yet reconnected at capture time.
- **Denied operations while stopped:** `wifi scan` and `wifi restart` correctly rejected with clear “service stopped/disabled…” guidance (no unintended bring-up evidenced).
- **A-B memory effect:** default free increased after stop (~69028 B → ~75644 B in `svc_mem`), then decreased after start/restart (~64548 B, larger-block fragmentation/lower min later) — useful point-in-time evidence, not a leak conclusion.
- **Still open:** STA reconnect completion in captured window, scan/connect while active, repeated cycles/stability, persisted-disabled reboot and profile-forced-off reboot cases, longer-run measurements.

## Coverage summary by validation item (from A/B/D)

| Item | Status from captures | Notes |
|---|---|---|
| 1. R22 M3 `light` | Partial, inconclusive | `light` returned `0` x3; no lighting variation, no clean explicit-path/API/error evidence |
| 2. R22 M4 `time` | Partial-positive | `time foo` fallthrough/full status correct and synced; clean `time`/`utc`/`epoch`/`system.time_info()` captures still needed |
| 3. R12 Milestone 4 | Partial data only | `ping`/`nslookup` worked; touch scrollback and clarified acceptance criteria pending |
| 4. Unified lifecycle smoke | Partial | Runtime states coherent; boot-log/default-startup comparison not captured |
| 5. Boot profiles | Partial baseline only | Default snapshots captured; low-memory-offline reboot/availability/recovery/memory A-B not captured |
| 6. Text console lifecycle | Not validated | Requires manual transitions/behavior checks |
| 7. svc_mem observability | Partial-positive | Repeated stable reads (A) and Wi-Fi stop/start/restart A-B captured (D); ranges/watermarks and broader cases pending |
| 8. Network diagnostics worker | Partial incidental | `ping`/`nslookup` ok, `diagnostics_worker=no` at idle; timeout/abandon/concurrent/lazy/profile-disabled cases not captured |
| 9. R20 pipes v1 | Inconclusive | Pipe/overflow commands ran but output/diagnostics not clearly evidenced in corrupted capture; retest needed |
| 10. Berry runtime/service inspection v1 | Partial-positive (idle) | Idle `services`/`berry app none`/memory snapshots ok; active app/close/varied states pending |
| 11. Bluetooth lazy scan state | Not validated (residual gaps) | No scan/connect/forget/cycle/memory/auto-enable-boot evidence |
| 12. Wi-Fi managed start/stop | Partial-positive | Stop/start/restart transitions, persistence/status, denied-while-stopped, and A-B heap effect evidenced; reconnect-completion/active ops/reboots/repeated cycles pending |

## Notes / gaps
- Section A paste corruption caused mangled commands and missing evidence — rerun affected checks in small chunks.
- Firmware commit/build, reboot boundaries, and manual observations were not fully recorded in captures.
- No claims made for unexecuted checks, merge, or complete hardware validation closure.

## Focused follow-up captures (R1-R8 partial, 2026-11-17, SSIDs redacted)

**Source:** User-provided serial console captures (chat). SSIDs masked in this section.

### R1 — R22 M3 `light` (partial)
```text
COS/flash> berry eval "print('--- R1: light baseline (change lighting between commands) ---')"
--- R1: light baseline (change lighting between commands) ---
COS/flash> light
Light sensor: 0
COS/flash> light
Light sensor: 0
COS/flash> light
Light sensor: 0
COS/flash> light
Light sensor: 0
COS/flash> light
Light sensor: 0
COS/flash> light
Light sensor: 0
COS/flash>light
Light sensor: 0
COS/flash> berry eval "print('--- R1: explicit path + API ---')"
--- R1: explicit path + API ---
COS/flash> /flash/scripts/light.be
Light sensor: 304
COS/flash> berry eval "var m = cos_import('@cos/light'); var l = m.find('default'); print(l.read())"
{'value': 304, 'ok': true}
COS/flash> berry eval "var m = cos_import('@cos/light'); var l = m.find('default'); print(l.read())"
{'value': 0, 'ok': true}
COS/flash> berry eval "var m = cos_import('@cos/light'); var l = m.find('default'); print(l.read())"
{'value': 0, 'ok': true}
COS/flash> berry eval "var m = cos_import('@cos/light'); var l = m.find('default'); print(l.read())"
{'value': 0, 'ok': true}
COS/flash> berry eval "var m = cos_import('@cos/light'); var l = m.find('default'); print(l.read())"
{'value': 0, 'ok': true}
COS/flash> berry eval "var m = cos_import('@cos/light'); var l = m.find('default'); print(l.read())"
{'value': 0, 'ok': true}
COS/flash> /flash/scripts/light.be
Light sensor: 0
COS/flash> /flash/scripts/light.be
Light sensor: 0
```
**Observation:** command/API/explicit script path executed successfully. Values changed (`0` and `304`), indicating sensor responsiveness, but lighting conditions were not described; later reads returned `0`. Error/unavailable path not reproduced (N/A). Evidence still partial/inconclusive for a fully recorded pass across lighting conditions.

### R2 — R22 M4 `time` (clean, partial-positive)
```text
COS/flash> berry eval "print('--- R2: time parity ---')"
--- R2: time parity ---
COS/flash> time
Local:  2026-11-17T17:32:53+01:00
UTC:    2026-11-17T16:32:52Z
Synced: yes
COS/flash> time utc
2026-11-17T16:32:58Z
COS/flash> time epoch
1794933182
COS/flash> time foo
Local:  2026-11-17T17:33:06+01:00
UTC:    2026-11-17T16:33:06Z
Synced: yes
COS/flash> berry eval "print('--- R2: system.time_info API ---')"
--- R2: system.time_info API ---
COS/flash> berry eval "var m = cos_import('@cos/system'); var s = m.find('default'); print(s.time_info())"
{'utc': '2026-11-17T16:33:13Z', 'local': '2026-11-17T17:33:13+01:00', 'ok': true, 'epoch': 1794933193, 'synced': true}
```
**Observation:** `time`, `time utc`, `time epoch`, unknown-arg fallthrough, and `system.time_info()` captured in synced state with expected fields/behavior. Formatting-failure path not reproduced (N/A). Strong evidence for M4 parity/API in this state; firmware commit/build identity not recorded in capture.

### R3 — R20 pipes v1 (mixed + notable findings)
```text
COS/flash> berry eval "print('--- R3: happy path (note any stdin/log output from pipe_stdin) ---')"
--- R3: happy path (note any stdin/log output from pipe_stdin) ---
COS/flash> berry eval "print('PIPE_HELLO')" | /flash/scripts/pipe_stdin.be
COS/flash> time | /flash/scripts/pipe_stdin.be
COS/flash> berry eval "print('--- R3: unsupported/non-Berry consumer ---')"
--- R3: unsupported/non-Berry consumer ---
COS/flash> berry eval "print('x')" | mem
Free heap:      63344 bytes
Min free heap:  58892 bytes
Total heap:     198200 bytes
Max alloc:      57344 bytes
Internal free:  63344 bytes
Internal min:   58892 bytes
Internal total: 198200 bytes
Internal max:   57344 bytes
COS/flash> berry eval "print('--- R3: 3-stage unsupported ---')"
--- R3: 3-stage unsupported ---
COS/flash> berry eval "print('a')" | /flash/scripts/pipe_stdin.be | /flash/scripts/pipe_stdin.be
pipes: not supported yet
COS/flash> berry eval "print('--- R3: pipe+redirection unsupported ---')"
--- R3: pipe+redirection unsupported ---
COS/flash> berry eval "print('x')" | /flash/scripts/pipe_stdin.be > /flash/pipe_out.txt
pipes: not supported yet
COS/flash> berry eval "print('--- R3: overflow (>8 KiB, expect clear diagnostic, rhs not run) ---')"
--- R3: overflow (>8 KiB, expect clear diagnostic, rhs not run) ---
COS/flash> berry eval "var i=0; while(i<300) print('01234567890123456789012345678'); i=i+1; end" | /flash/scripts/pipe_stdin.be
pipes: input too large (max 8192 bytes)
```
**Observations:**
- Happy-path pipelines ran without visible `stdin:` text in capture (`pipe_stdin.be` uses `log()`); stdin delivery not directly evidenced from console text.
- **Notable finding:** `berry eval "print('x')" | mem` executed `mem` and printed heap output instead of a handled unsupported-consumer diagnostic (v1 expects Berry-script-only `rhs`). Needs investigation/retest.
- 3-stage and pipe+redirection correctly reported `pipes: not supported yet`.
- Overflow correctly reported `pipes: input too large (max 8192 bytes)`.

### R4 — Wi-Fi reconnect/active check (partial-positive)
```text
COS/flash> berry eval "print('--- R4: reconnect/active check ---')"
--- R4: reconnect/active check ---
COS/flash> wifi status
Service: started
User enabled (persisted): yes
Boot profile: allowed
Effective: enabled
Mode:    AP+STA
AP IP:   192.168.4.1
STA:     <REDACTED_SSID>
STA IP:  192.168.0.187
Signal:  -61 dBm (Very good)
MAC:     34:5F:45:AA:CF:40
COS/flash> services
[http_server] HTTP server: state=running
[websocket] WebSocket: state=running clients=0 http_server=yes
[wifi] Wi-Fi: state=connected connected=yes stored_credentials=yes
[bluetooth] Bluetooth: state=unknown enabled=no enabling=no connected=no
[network] Network: state=idle diagnostics_worker=no
[time_sync] Time sync: state=ready synced=yes
COS/flash> svc_mem
(svc_mem snapshot captured; Wi-Fi connected, heap/runtime fields reported)
COS/flash> ping 8.8.8.8
Reply from 8.8.8.8: time=42 ms
COS/flash> wifi scan
SSID                      RSSI  Signal       Auth
                           -57  Very good    WPA2_PSK
<REDACTED_SSID>                     -62  Very good    WPA2_PSK
<REDACTED_SSID_IOT>                 -62  Very good    WPA2_PSK
ESP-E097AF                 -63  Very good    OPEN
AutoConnectAP              -73  Not good     OPEN
ESP-EE2371                 -76  Not good     OPEN
                           -77  Not good     OPEN
                           -90  Unknown      WPA2_PSK
flea                       -92  Unknown      WPA2_PSK
Yettel-B85A                -92  Unknown      WPA2_PSK
Telekom-967759             -92  Unknown      WPA2_WPA3_PSK
Telekom-967759             -93  Unknown      WPA2_WPA3_PSK
ESP-AA8D5A                 -94  Unknown      OPEN
Telekom-Hotspot            -94  Unknown      WPA2_ENTERPRISE
14 network(s) found
```
**Observation:** STA connected (IP assigned), ping succeeded, active `wifi scan` succeeded (SSIDs masked here).

### R5 — Berry app active/close (partial-positive, SD card path)
```text
COS/flash> berry eval "print('--- R5: list apps ---')"
--- R5: list apps ---
COS/flash> ls /sdcard/apps
      0  breakout.be
      0  tap_dash.be
2 file(s), 0 dir(s), 0 bytes
COS/flash> berry run /sdcard/apps/breakout.be
COS/flash> berry eval "print('--- R5: while app active ---')"
--- R5: while app active ---
COS/flash> berry app
path=/sdcard/apps/breakout.be type=ui state=running
COS/flash> services
[http_server] HTTP server: state=running
[websocket] WebSocket: state=running clients=0 http_server=yes
[wifi] Wi-Fi: state=connected connected=yes stored_credentials=yes
[bluetooth] Bluetooth: state=unknown enabled=no enabling=no connected=no
[network] Network: state=idle diagnostics_worker=no
[time_sync] Time sync: state=ready synced=yes
COS/flash> berry eval "print('--- R5: close app ---')"
--- R5: close app ---
COS/flash> berry close
COS/flash> berry app
none
```
**Observation:** active UI app path/state reported while running from SD card; `berry close` returned state to `none`.

### R6 — `low-memory-offline` profile (critical issue observed)
```text
COS/flash> berry eval "print('--- R6.1: set low-memory-offline ---')"
--- R6.1: set low-memory-offline ---
COS/flash> boot_profile set low-memory-offline
Stored boot profile set to 'low-memory-offline'. Takes effect on next boot/restart.
Effective profile for current boot remains: default
COS/flash> restart
(restart/boot to low-memory-offline)

COS/flash> boot_profile
Stored:    low-memory-offline
Effective: low-memory-offline (current boot)
Next boot: low-memory-offline
Profiles:  default, low-memory-offline
Profile policy: Wi-Fi, network diagnostics, HTTP server, WebSocket, time sync, and Bluetooth stack/radio are forced off/unavailable for this boot (Bluetooth init still runs for compatibility/recovery; stored BT auto-enable preserved).

COS/flash> services

assert failed: xQueueSemaphoreTake queue.c:1709 (( pxQueue ))


Backtrace: 0x40093469:0x3ffdb3f0 0x40093431:0x3ffdb410 0x40092e91:0x3ffdb430 0x40239522:0x3ffdb550 0x401269e9:0x3ffdb590 0x400e5d43:0x3ffdb5b0 0x400e60a0:0x3ffdb5d0 0x400e5676:0x3ffdb5f0 0x40147db7:0x3ffdb7b0 0x400e1f0d:0x3ffdb7d0 0x400e2453:0x3ffdb840 0x400e3631:0x3ffdba90 0x400e3696:0x3ffdbb40 0x400e3870:0x3ffdbb70 0x400e38fb:0x3ffdbb90 0x400e394c:0x3ffdbbb0 0x40238d56:0x3ffdbbf0
--- 0x40093469: panic_abort at C:/esp/v6.1/esp-idf/components/esp_system/panic.c:500
--- 0x40093431: esp_system_abort at C:/esp/v6.1/esp-idf/components/esp_system/port/esp_system_chip.c:87
--- 0x40092e91: __assert_func at C:/esp/v6.1/esp-idf/components/esp_libc/src/assert.c:81
--- 0x40239522: xQueueSemaphoreTake at C:/esp/v6.1/esp-idf/components/freertos/FreeRTOS-Kernel/queue.c:1709
--- 0x401269e9: websocket_client_count at E:/dev/cos/components/websocket/src/websocket.c:96
--- 0x400e5d43: collect_websocket at E:/dev/cos/components/service_diagnostics/src/service_diagnostics.c:299
--- 0x400e60a0: service_diag_collect at E:/dev/cos/components/service_diagnostics/src/service_diagnostics.c:390
--- 0x400e5676: cmd_services at E:/dev/cos/components/shell/src/shell_services_cmd.c:59
--- 0x40147db7: esp_console_run at C:/esp/v6.1/esp-idf/components/console/commands.c:258
--- 0x400e1f0d: shell_dispatch_command_line at E:/dev/cos/components/shell/src/shell_script.c:467
--- 0x400e2453: shell_dispatch_expanded_line at E:/dev/cos/components/shell/src/shell_script.c:844
--- 0x400e3631: execute_line at E:/dev/cos/components/shell/src/shell_input.c:284
--- 0x400e3696: handle_normal_char at E:/dev/cos/components/shell/src/shell_input.c:319
--- 0x400e3870: process_byte at E:/dev/cos/components/shell/src/shell_input.c:728
--- 0x400e38fb: process_input_buffer at E:/dev/cos/components/shell/src/shell_input.c:767
--- 0x400e394c: input_task at E:/dev/cos/components/shell/src/shell_input.c:825
--- 0x40238d56: vPortTaskWrapper at C:/esp/v6.1/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:147

ELF file SHA256: 6b9787961

Rebooting...

COS/flash> bt
Bluetooth: forced off by boot profile (low-memory-offline, current boot, stored auto-enable: off)
COS/flash> mem
Free heap:      135308 bytes
Min free heap:  133020 bytes
Total heap:     198200 bytes
Max alloc:      110592 bytes
Internal free:  135308 bytes
Internal min:   133020 bytes
Internal total: 198200 bytes
Internal max:   110592 bytes

COS/flash> berry profile
Berry memory profile (R2/R5/R8)
  Default heap:  free=134100 min=132932 total=198200 largest=110592
  Internal heap: free=134100 min=132932 total=198200 largest=110592
  Berry GC:      initialized=yes managed=6159 bytes step_rate=200
  Heap snapshot: free=134100 min=132932 largest=110592 internal_free=160176 internal_min=158976 internal_largest=110592
  HTTP worker:   not started (cfg=6144 unit=1)

COS/flash> svc_mem

assert failed: xQueueSemaphoreTake queue.c:1709 (( pxQueue ))


Backtrace: 0x40093469:0x3ffdb3f0 0x40093431:0x3ffdb410 0x40092e91:0x3ffdb430 0x40239522:0x3ffdb550 0x401269e9:0x3ffdb590 0x400e5d43:0x3ffdb5b0 0x400e60a0:0x3ffdb5d0 0x400e60c5:0x3ffdb5f0 0x400e4ea3:0x3ffdb790 0x40147db7:0x3ffdb7b0 0x400e1f0d:0x3ffdb7d0 0x400e2453:0x3ffdb840 0x400e3631:0x3ffdba90 0x400e3696:0x3ffdbb40 0x400e3870:0x3ffdbb70 0x400e38fb:0x3ffdbb90 0x400e394c:0x3ffdbbb0 0x40238d56:0x3ffdbbf0
--- 0x40093469: panic_abort at C:/esp/v6.1/esp-idf/components/esp_system/panic.c:500
--- 0x40093431: esp_system_abort at C:/esp/v6.1/esp-idf/components/esp_system/port/esp_system_chip.c:87
--- 0x40092e91: __assert_func at C:/esp/v6.1/esp-idf/components/esp_libc/src/assert.c:81
--- 0x40239522: xQueueSemaphoreTake at C:/esp/v6.1/esp-idf/components/freertos/FreeRTOS-Kernel/queue.c:1709
--- 0x401269e9: websocket_client_count at E:/dev/cos/components/websocket/src/websocket.c:96
--- 0x400e5d43: collect_websocket at E:/dev/cos/components/service_diagnostics/src/service_diagnostics.c:299
--- 0x400e60a0: service_diag_collect at E:/dev/cos/components/service_diagnostics/src/service_diagnostics.c:390
--- 0x400e60c5: service_diag_print_svc_mem at E:/dev/cos/components/service_diagnostics/src/service_diagnostics.c:433
--- 0x400e4ea3: cmd_svc_mem at E:/dev/cos/components/shell/src/shell_info_cmds.c:33
--- 0x40147db7: esp_console_run at C:/esp/v6.1/esp-idf/components/console/commands.c:258
--- 0x400e1f0d: shell_dispatch_command_line at E:/dev/cos/components/shell/src/shell_script.c:467
--- 0x400e2453: shell_dispatch_expanded_line at E:/dev/cos/components/shell/src/shell_script.c:844
--- 0x400e3631: execute_line at E:/dev/cos/components/shell/src/shell_input.c:284
--- 0x400e3696: handle_normal_char at E:/dev/cos/components/shell/src/shell_input.c:319
--- 0x400e3870: process_byte at E:/dev/cos/components/shell/src/shell_input.c:728
--- 0x400e38fb: process_input_buffer at E:/dev/cos/components/shell/src/shell_input.c:767
--- 0x400e394c: input_task at E:/dev/cos/components/shell/src/shell_input.c:825
--- 0x40238d56: vPortTaskWrapper at C:/esp/v6.1/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:147

ELF file SHA256: 6b9787961

Rebooting...

COS/flash> berry eval "print('--- R6.3: restore default ---')"
--- R6.3: restore default ---
COS/flash> boot_profile clear
Stored boot profile cleared. Next boot: default (takes effect on next boot/restart)
COS/flash> boot_profile
Stored:    default (none/invalid -> default)
Effective: low-memory-offline (current boot)
Next boot: default
Profiles:  default, low-memory-offline
```
**Critical findings:**
- In `low-memory-offline`, both `services` and `svc_mem` triggered assert/panic via `websocket_client_count` → service diagnostics collection (`collect_websocket`), causing reboot. Diagnostics/inspection commands appear unsafe when WebSocket is unavailable/not initialized in this profile.
- `boot_profile` reported stored/effective/next state and policy correctly; `bt` reported forced-off-by-profile with stored auto-enable preserved.
- Heap was much larger in low-memory-offline before crash, but A-B/memory validation is incomplete due to instability.
- `boot_profile clear` set next boot to default while effective remained low-memory-offline for current boot (expected). Post-restore reboot/default confirmation not fully captured.

### R8 — Bluetooth baseline (default profile)
```text
COS/flash> berry eval "print('--- R8: BT baseline ---')"
--- R8: BT baseline ---
COS/flash> bt status
Usage: bt [on | off | scan | connect <addr> | disconnect | forget]
COS/flash> bt
Bluetooth: OFF
COS/flash> services
[http_server] HTTP server: state=running
[websocket] WebSocket: state=running clients=0 http_server=yes
[wifi] Wi-Fi: state=connected connected=yes stored_credentials=yes
[bluetooth] Bluetooth: state=unknown enabled=no enabling=no connected=no
[network] Network: state=idle diagnostics_worker=no
[time_sync] Time sync: state=ready synced=yes
COS/flash> mem
Free heap:      66800 bytes
Min free heap:  63560 bytes
Total heap:     198200 bytes
Max alloc:      63488 bytes
Internal free:  66800 bytes
Internal min:   63560 bytes
Internal total: 198200 bytes
Internal max:   63488 bytes
COS/flash> svc_mem
(svc_mem snapshot captured; BT unknown/disabled, services up)
```
**Observation:** baseline BT-off state captured in default profile; no cycle/connect/forget/auto-enable-boot evidence.

### Incremental coverage notes
- **R22 M3:** improved (value change 0/304 observed) but still partial without documented lighting sweep/error path.
- **R22 M4:** clean parity/API capture in synced state (strong evidence; commit/build identity not recorded).
- **R20 pipes:** 3-stage/pipe+redirect/overflow diagnostics evidenced; happy-path stdin delivery not visibly confirmed; **unexpected behavior for non-Berry `rhs` consumer (`| mem`)** needs investigation.
- **R4/Item 12:** reconnect, active ping, active scan confirmed.
- **R5/Item 10:** active app and close confirmed.
- **R6/Items 5 & diagnostics:** **critical low-memory-offline crash in `services`/`svc_mem`** blocks profile inspection/validation.
- **R8/Item 11:** baseline only.
