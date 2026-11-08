# R2 / P7 Profiling & Regression Checklist (2026-11-06 memory pressure fixes)

**Date:** 2026-11-08  
**Scope:** Implements the R2 "measurement method/script/checklist committed" item from `docs/2026-11-08-consolidated-improvement-plan.md` (source: `docs/2026-11-06-memory-pressure-fixes.md` P7).  
**Status:** Checklist committed. Baseline numbers and regression execution must be recorded on hardware / with an ESP-IDF build.

## Prerequisites

- ESP-IDF environment loaded (PowerShell example): run ESP-IDF `export.ps1` / `install.ps1` environment as usual for this repo.
- Target hardware connected with serial monitor access.
- Firmware built from the branch/commit under test.
- For network regression cases: device connected to Wi-Fi with working DNS/HTTP/HTTPS as applicable.
- For FS regression cases: scripts deployed to `/flash/tests/r2/` (included in `storage/flash_data` image) or copied to `/sdcard/tests/r2/`.

## 1. Static memory / `.bss` comparison

Run from `E:\dev\cos`:

```powershell
idf.py size
idf.py size-components
```

Record in the report below:

- Total DRAM/IRAM/Flash summary from `idf.py size`.
- `berry_lang` and `network` component rows from `idf.py size-components` (or the equivalent detailed output).
- `.bss` impact for `berry_lang` and `network` (use `idf.py size-components`, `idf.py size-files`, or inspect `build/*.map` for the relevant objects if needed).

Optional detailed map inspection:

```powershell
idf.py size-files
# or inspect build/cos.map for berry_lang / network .bss symbols
```

## 2. Runtime metrics to capture

P7 metrics (per checkpoint, where practical):

| Metric                                                        | How                                                  |
| ------------------------------------------------------------- | ---------------------------------------------------- |
| Free heap (default caps)                                      | `mem` or `berry profile` → `System heap: free=`      |
| Minimum free heap / watermark (default)                       | `mem` or `berry profile` → `System heap: min=`       |
| Largest allocatable block (default)                           | `mem` or `berry profile` → `System heap: largest=`   |
| Free heap (internal `MALLOC_CAP_INTERNAL \| MALLOC_CAP_8BIT`) | `mem` or `berry profile` → `Internal heap: free=`    |
| Minimum free heap (internal watermark)                        | `mem` or `berry profile` → `Internal heap: min=`     |
| Largest internal block                                        | `mem` or `berry profile` → `Internal heap: largest=` |
| Berry GC managed bytes                                        | `berry profile` → `Berry GC: managed=`               |

Primary capture commands:

```text
mem
berry profile
berry gc            # checkpoint 9 / when explicit GC is useful; prints before/after and profile
```

Boot-time checkpoints (1–2) may need serial boot logs; `components/berry_lang/src/berry_runtime.cpp` logs `Berry VM initialized: ...` and other heap snapshots via `log_berry_heap`/`log_heap_snapshot`.

## 3. P7 checkpoint matrix

Fill after hardware measurement. Use "N/A" with a reason if a checkpoint is not observable from the shell.

|   # | Checkpoint                                  | Method                                                                                                                                                                       | Free (sys) | Min (sys) | Largest (sys) | Free (int) | Min (int) | Largest (int) | Berry GC bytes | Notes                                                                                                           |
| --: | ------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------: | --------: | ------------: | ---------: | --------: | ------------: | -------------: | --------------------------------------------------------------------------------------------------------------- |
|   1 | System startup, before Berry initialization | Serial boot log / N/A from shell                                                                                                                                             |            |           |               |            |           |               |                | `berry_runtime_init` is called during boot; capture from early boot logs if instrumented/available.             |
|   2 | Immediately after `berry_runtime_init()`    | Serial boot log (`Berry VM initialized`) or first `berry profile` after boot                                                                                                 |            |           |               |            |           |               |                | Compare with R1 expectation: no eager JSON load at startup.                                                     |
|   3 | After opening a normal Berry application    | `berry run /flash/apps/...` then `berry profile` (from a context that can run) or observe logs; alternatively run app and capture before `berry close` via instrumented flow |            |           |               |            |           |               |                | If shell is not available while app owns UI, capture before/after via logs and `berry close` + `berry profile`. |
|   4 | After executing `fs.read_text()`            | `berry run /flash/tests/r2/fs_text.be` or eval/script, then `berry profile`                                                                                                  |            |           |               |            |           |               |                | Use `storage/flash_data/tests/r2/fs_text.be`.                                                                   |
|   5 | After executing `fs.read_json()`            | `berry run /flash/tests/r2/fs_json.be`, then `berry profile`                                                                                                                 |            |           |               |            |           |               |                | Validates lazy JSON load path is exercised.                                                                     |
|   6 | After the first HTTP GET                    | `berry run /flash/tests/r2/http_get.be <url>` or manual eval with known HTTP endpoint, then `berry profile`                                                                  |            |           |               |            |           |               |                | Record URL/status; requires Wi-Fi.                                                                              |
|   7 | After an HTTPS GET                          | `berry run /flash/tests/r2/https_get.be <url>`, then `berry profile`                                                                                                         |            |           |               |            |           |               |                | Uses TLS/cert bundle; do not disable certificate verification.                                                  |
|   8 | After a maximum-size HTTP response          | Endpoint/script that approaches `NETWORK_HTTP_BODY_MAX` / max response handling, then `berry profile`                                                                        |            |           |               |            |           |               |                | Record actual response size/status; may require controlled server.                                              |
|   9 | After Berry garbage collection              | `berry gc`                                                                                                                                                                   |            |           |               |            |           |               |                | `berry gc` prints before/after/reclaimed and a fresh profile.                                                   |
|  10 | After repeated network requests             | `berry run /flash/tests/r2/repeated_fetch.be <url> <count>`, then `berry profile` / `berry gc`                                                                               |            |           |               |            |           |               |                | Look for unexplained downward trend in free heap/largest block after GC.                                        |

## 4. Regression matrix (P7 required cases)

| Case                                                                     | Command/script                                                                          | Expected result                                                                  | Pass | Notes                                                         |
| ------------------------------------------------------------------------ | --------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------- | :--: | ------------------------------------------------------------- |
| Plain Berry apps work without JSON loaded at startup                     | Boot, run non-JSON app/script, `berry profile`                                          | No startup regression; JSON not eagerly loaded (R1)                              |      |                                                               |
| Filesystem text operations work                                          | `berry run /flash/tests/r2/fs_text.be`                                                  | Prints/returns OK; file contents read/written as expected                        |      |                                                               |
| Filesystem JSON operations work                                          | `berry run /flash/tests/r2/fs_json.be`                                                  | `read_json`/`write_json` OK; lazy JSON loads on demand                           |      |                                                               |
| HTTP text responses work                                                 | `net.fetch`/`wget`/test script with `response_type="text"`                              | Status/body/error shape correct                                                  |      |                                                               |
| HTTP JSON responses work                                                 | `net.fetch(..., response_type="json")`                                                  | `ok/status/json_ok/json` correct                                                 |      |                                                               |
| HTTPS certificate verification still works                               | HTTPS GET to valid cert endpoint                                                        | Success with verification; invalid cert should fail (separate check if feasible) |      |                                                               |
| Large request bodies work                                                | POST with body near supported limit                                                     | Accepted/rejected per limits; no corruption                                      |      |                                                               |
| Maximum-size response handling remains correct                           | Response near configured max                                                            | Truncated/handled per implementation; no crash                                   |      |                                                               |
| Timeout does not cause use-after-free                                    | Request with short timeout / unreachable host                                           | Clean error, no crash/hang                                                       |      |                                                               |
| Timed-out request does not allow buffer overwrite while worker uses them | Repeat timeout scenarios; inspect logs/crashes                                          | No memory corruption                                                             |      |                                                               |
| Repeated HTTP requests show no progressive heap decrease after GC        | `repeated_fetch.be` + `berry gc` + `berry profile`                                      | Stable after warmup/GC within noise                                              |      |                                                               |
| HTTP worker stack remains safe under worst-case conditions               | GET/POST/HTTPS/max response/timeout/error paths; check for stack canary/watchdog issues | No stack overflow symptoms                                                       |      | Full high-water-mark measurement is R8/P4; R2 records sanity. |

## 5. Report template (paste results here or in a linked report)

**Build/commit:**  
**Target/hardware:**  
**Wi-Fi/network environment:**  
**Date/tester:**

### `idf.py size` summary

```text
+----------------------------------------------------------------------------------+
| Memory Type/Section   | Used [bytes] | Used [%] | Remain [bytes] | Total [bytes] |
|-----------------------+--------------+----------+----------------+---------------|
| Flash Code            |      1426174 |          |                |               |
|    .text              |      1426174 |          |                |               |
| Flash Data            |       368064 |          |                |               |
|    .rodata            |       367784 |          |                |               |
|    .appdesc           |          256 |          |                |               |
|    .tdata             |           24 |          |                |               |
| IRAM                  |       107115 |    81.72 |          23957 |        131072 |
|    .text              |       106087 |    80.94 |                |               |
|    .vectors           |         1028 |     0.78 |                |               |
| DRAM                  |       101835 |    81.74 |          22745 |        124580 |
|    .bss               |        75728 |    60.79 |                |               |
|    .data              |        26107 |    20.96 |                |               |
| RTC SLOW              |           64 |     0.78 |           8128 |          8192 |
|    .force_slow        |           40 |     0.49 |                |               |
|    .rtc_slow_reserved |           24 |     0.29 |                |               |
+----------------------------------------------------------------------------------+
```

### `idf.py size-components` relevant rows

```text
+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Archive File              | Total Size |  DRAM |  .bss | .data |  IRAM | .text | .vectors | Flash Code |  .text | Flash Data | .tdata | .rodata | .appdesc | RTC SLOW | .rtc_slow_reserved | .force_slow |
|---------------------------+------------+-------+-------+-------+-------+-------+----------+------------+--------+------------+--------+---------+----------+----------+--------------------+-------------|
| libbt.a                   |     281793 | 15833 | 11117 |  4716 |  1595 |  1595 |        0 |     246676 | 246676 |      17689 |      0 |   17689 |        0 |        0 |                  0 |           0 |
| libesp_stdio.a            |     213107 |    40 |     8 |    32 |     0 |     0 |        0 |        963 |    963 |     212104 |      0 |  212104 |        0 |        0 |                  0 |           0 |
| libnet80211.a             |     170771 | 10725 |  9853 |   872 |     0 |     0 |        0 |     143866 | 143866 |      16180 |      0 |   16180 |        0 |        0 |                  0 |           0 |
| libtfpsacrypto.a          |     123755 |   337 |   265 |    72 |     0 |     0 |        0 |     106628 | 106628 |      16790 |      0 |   16790 |        0 |        0 |                  0 |           0 |
| libberry_lang.a           |     121143 |  5983 |  5983 |     0 |     0 |     0 |        0 |     106769 | 106769 |       8391 |      0 |    8391 |        0 |        0 |                  0 |           0 |
| liblwip.a                 |     115176 |  3790 |  3774 |    16 |     0 |     0 |        0 |     107197 | 107197 |       4189 |      0 |    4189 |        0 |        0 |                  0 |           0 |
| libwpa_supplicant.a       |      82253 |  1715 |  1707 |     8 |     0 |     0 |        0 |      78674 |  78674 |       1864 |      0 |    1864 |        0 |        0 |                  0 |           0 |
| libbtdm_app.a             |      79823 |  3716 |  2694 |  1022 | 22713 | 22713 |        0 |      52749 |  52749 |        645 |      0 |     645 |        0 |        0 |                  0 |           0 |
| libpp.a                   |      72468 |  4211 |  1572 |  2639 |  1951 |  1951 |        0 |      61654 |  61654 |       4652 |      0 |    4652 |        0 |        0 |                  0 |           0 |
| liblovyanGFX.a            |      51992 |   656 |   572 |    84 |     0 |     0 |        0 |      35148 |  35148 |      16188 |      0 |   16188 |        0 |        0 |                  0 |           0 |
| libphy.a                  |      44945 |  1225 |   641 |   584 |  9118 |  9118 |        0 |      34602 |  34602 |          0 |      0 |       0 |        0 |        0 |                  0 |           0 |
| libc.a                    |      40994 |   548 |   528 |    20 |     0 |     0 |        0 |      36193 |  36193 |       4253 |      0 |    4253 |        0 |        0 |                  0 |           0 |
| libjoltwallet__littlefs.a |      31943 |   140 |    16 |   124 |     0 |     0 |        0 |      30869 |  30869 |        934 |      0 |     934 |        0 |        0 |                  0 |           0 |
| libfreertos.a             |      25374 |  3856 |   748 |  3108 |  5729 |  5729 |        0 |      14069 |  14069 |       1720 |      0 |    1720 |        0 |        0 |                  0 |           0 |
| libmbedtls.a              |      25194 |    72 |    72 |     0 |     0 |     0 |        0 |      24304 |  24304 |        818 |      0 |     818 |        0 |        0 |                  0 |           0 |
| libfatfs.a                |      24799 |    64 |    60 |     4 |     0 |     0 |        0 |      23235 |  23235 |       1500 |      0 |    1500 |        0 |        0 |                  0 |           0 |
| libnetwork.a              |      24619 | 15802 | 15802 |     0 |     0 |     0 |        0 |       8816 |   8816 |          1 |      0 |       1 |        0 |        0 |                  0 |           0 |
| libesp_hw_support.a       |      22349 |   596 |   148 |   448 |  9444 |  9444 |        0 |      10981 |  10981 |       1264 |      0 |    1264 |        0 |       64 |                 24 |          40 |
| libtext_console.a         |      20522 | 13080 | 13066 |    14 |     0 |     0 |        0 |       7395 |   7395 |         47 |      0 |      47 |        0 |        0 |                  0 |           0 |
| libspi_flash.a            |      19524 |  7332 |    24 |  7308 |  9630 |  9630 |        0 |       1267 |   1267 |       1295 |      0 |    1295 |        0 |        0 |                  0 |           0 |
| libmbedtls.a              |      19034 |     4 |     4 |     0 |     0 |     0 |        0 |       1336 |   1336 |      17694 |      0 |   17694 |        0 |        0 |                  0 |           0 |
| libshell.a                |      15910 |  2027 |  1883 |   144 |     0 |     0 |        0 |      13684 |  13684 |        199 |      0 |     199 |        0 |        0 |                  0 |           0 |
| libesp_rom.a              |      15892 |     0 |     0 |     0 |   423 |   423 |        0 |         44 |     44 |      15425 |      0 |   15425 |        0 |        0 |                  0 |           0 |
| libnvs_flash.a            |      15709 |    24 |    24 |     0 |     0 |     0 |        0 |      15429 |  15429 |        256 |      0 |     256 |        0 |        0 |                  0 |           0 |
| libsdmmc.a                |      15676 |     0 |     0 |     0 |     0 |     0 |        0 |      14148 |  14148 |       1528 |      0 |    1528 |        0 |        0 |                  0 |           0 |
| libesp_http_server.a      |      15561 |     0 |     0 |     0 |     0 |     0 |        0 |      15005 |  15005 |        556 |      0 |     556 |        0 |        0 |                  0 |           0 |
| libhttp_parser.a          |      15215 |     0 |     0 |     0 |     0 |     0 |        0 |      14463 |  14463 |        752 |      0 |     752 |        0 |        0 |                  0 |           0 |
| libesp_system.a           |      14258 |   754 |   321 |   433 |  3859 |  3859 |        0 |       9025 |   9025 |        620 |      0 |     620 |        0 |        0 |                  0 |           0 |
| libheap.a                 |      13419 |    21 |     8 |    13 |  7768 |  7768 |        0 |       3939 |   3939 |       1691 |      0 |    1691 |        0 |        0 |                  0 |           0 |
| libesp_driver_spi.a       |      13181 |   196 |    45 |   151 |  4407 |  4407 |        0 |       7873 |   7873 |        705 |      0 |     705 |        0 |        0 |                  0 |           0 |
| libesp_driver_i2c.a       |      11472 |    28 |    28 |     0 |   704 |   704 |        0 |      10352 |  10352 |        388 |      0 |     388 |        0 |        0 |                  0 |           0 |
| libesp_driver_uart.a      |      10621 |   361 |    33 |   328 |     0 |     0 |        0 |       9735 |   9735 |        525 |      0 |     525 |        0 |        0 |                  0 |           0 |
| libcoexist.a              |      10407 |   653 |     1 |   652 |  3263 |  3263 |        0 |       5297 |   5297 |       1194 |      0 |    1194 |        0 |        0 |                  0 |           0 |
| libesp_http_client.a      |      10032 |     0 |     0 |     0 |     0 |     0 |        0 |       9709 |   9709 |        323 |      0 |     323 |        0 |        0 |                  0 |           0 |
| libdisplay.a              |       9977 |   948 |   946 |     2 |     0 |     0 |        0 |       7868 |   7868 |       1161 |      0 |    1161 |        0 |        0 |                  0 |           0 |
| libesp_hid.a              |       9126 |    69 |    69 |     0 |     0 |     0 |        0 |       8932 |   8932 |        125 |      0 |     125 |        0 |        0 |                  0 |           0 |
| libhttp_server.a          |       8663 |   170 |    10 |   160 |     0 |     0 |        0 |       8463 |   8463 |         30 |      0 |      30 |        0 |        0 |                  0 |           0 |
| libbluetooth.a            |       8170 |  1557 |  1557 |     0 |     0 |     0 |        0 |       6294 |   6294 |        319 |      0 |     319 |        0 |        0 |                  0 |           0 |
| libmbedx509.a             |       8035 |     0 |     0 |     0 |     0 |     0 |        0 |       7337 |   7337 |        698 |      0 |     698 |        0 |        0 |                  0 |           0 |
| libesp_netif.a            |       7378 |   197 |   189 |     8 |     0 |     0 |        0 |       6927 |   6927 |        254 |      0 |     254 |        0 |        0 |                  0 |           0 |
| libesp_driver_gpio.a      |       7268 |    36 |     0 |    36 |     0 |     0 |        0 |       6480 |   6480 |        752 |      0 |     752 |        0 |        0 |                  0 |           0 |
| libconsole.a              |       6842 |    48 |    20 |    28 |     0 |     0 |        0 |       6786 |   6786 |          8 |      0 |       8 |        0 |        0 |                  0 |           0 |
| libesp-tls.a              |       6505 |     4 |     4 |     0 |     0 |     0 |        0 |       6409 |   6409 |         92 |      0 |      92 |        0 |        0 |                  0 |           0 |
| libvfs.a                  |       6391 |   236 |    44 |   192 |     0 |     0 |        0 |       6016 |   6016 |        139 |      0 |     139 |        0 |        0 |                  0 |           0 |
| libesp_driver_ledc.a      |       5924 |    92 |    84 |     8 |   108 |   108 |        0 |       5472 |   5472 |        252 |      0 |     252 |        0 |        0 |                  0 |           0 |
| libesp_libc.a             |       5668 |  1068 |   668 |   400 |  1507 |  1507 |        0 |       2841 |   2841 |        252 |     24 |     228 |        0 |        0 |                  0 |           0 |
| libesp_wifi.a             |       5584 |   531 |    47 |   484 |   341 |   341 |        0 |       3885 |   3885 |        827 |      0 |     827 |        0 |        0 |                  0 |           0 |
| libesp_mm.a               |       5444 |   204 |   128 |    76 |  1415 |  1415 |        0 |       3589 |   3589 |        236 |      0 |     236 |        0 |        0 |                  0 |           0 |
| libui.a                   |       5424 |    97 |    57 |    40 |     0 |     0 |        0 |       5040 |   5040 |        287 |      0 |     287 |        0 |        0 |                  0 |           0 |
| libesp_ringbuf.a          |       5290 |     0 |     0 |     0 |  2952 |  2952 |        0 |       1800 |   1800 |        538 |      0 |     538 |        0 |        0 |                  0 |           0 |
| libesp_driver_sdspi.a     |       4829 |    17 |    17 |     0 |     0 |     0 |        0 |       4529 |   4529 |        283 |      0 |     283 |        0 |        0 |                  0 |           0 |
| libstdc++.a               |       4826 |    57 |    49 |     8 |     0 |     0 |        0 |       3904 |   3904 |        865 |      0 |     865 |        0 |        0 |                  0 |           0 |
| libfilesystem.a           |       4454 |     6 |     6 |     0 |     0 |     0 |        0 |       4344 |   4344 |        104 |      0 |     104 |        0 |        0 |                  0 |           0 |
| libtcp_transport.a        |       3906 |     0 |     0 |     0 |     0 |     0 |        0 |       3776 |   3776 |        130 |      0 |     130 |        0 |        0 |                  0 |           0 |
| libesp_event.a            |       3884 |     4 |     4 |     0 |     0 |     0 |        0 |       3717 |   3717 |        163 |      0 |     163 |        0 |        0 |                  0 |           0 |
| libesp_timer.a            |       3484 |    56 |    24 |    32 |  1440 |  1440 |        0 |       1857 |   1857 |        131 |      0 |     131 |        0 |        0 |                  0 |           0 |
| libxtensa.a               |       3417 |  1044 |     0 |  1044 |  2220 |  1793 |      427 |        117 |    117 |         36 |      0 |      36 |        0 |        0 |                  0 |           0 |
| libesp_hal_gpspi.a        |       3246 |    48 |     0 |    48 |  2885 |  2885 |        0 |        205 |    205 |        108 |      0 |     108 |        0 |        0 |                  0 |           0 |
| libwifi.a                 |       3229 |    12 |    12 |     0 |     0 |     0 |        0 |       3161 |   3161 |         56 |      0 |      56 |        0 |        0 |                  0 |           0 |
| libesp_hal_mspi.a         |       2865 |    36 |     0 |    36 |  2325 |  2325 |        0 |        504 |    504 |          0 |      0 |       0 |        0 |        0 |                  0 |           0 |
| libesp_phy.a              |       2762 |    53 |    36 |    17 |   280 |   280 |        0 |       2170 |   2170 |        259 |      0 |     259 |        0 |        0 |                  0 |           0 |
| libhal.a                  |       2438 |   245 |     9 |   236 |  2193 |  2193 |        0 |          0 |      0 |          0 |      0 |       0 |        0 |        0 |                  0 |           0 |
| liblog.a                  |       2286 |   284 |   276 |     8 |   503 |   503 |        0 |       1451 |   1451 |         48 |      0 |      48 |        0 |        0 |                  0 |           0 |
| libesp_hal_ana_conv.a     |       2244 |     0 |     0 |     0 |     0 |     0 |        0 |       2108 |   2108 |        136 |      0 |     136 |        0 |        0 |                  0 |           0 |
| libbootloader_support.a   |       2187 |     0 |     0 |     0 |  2034 |  2034 |        0 |        113 |    113 |         40 |      0 |      40 |        0 |        0 |                  0 |           0 |
| libesp_hal_gpio.a         |       2127 |    40 |     0 |    40 |     0 |     0 |        0 |       1034 |   1034 |       1053 |      0 |    1053 |        0 |        0 |                  0 |           0 |
| libesp_partition.a        |       2071 |     8 |     8 |     0 |     0 |     0 |        0 |       1895 |   1895 |        168 |      0 |     168 |        0 |        0 |                  0 |           0 |
| librtc.a                  |       2020 |     4 |     4 |     0 |  2016 |  2016 |        0 |          0 |      0 |          0 |      0 |       0 |        0 |        0 |                  0 |           0 |
| libesp_common.a           |       1817 |     0 |     0 |     0 |     0 |     0 |        0 |         51 |     51 |       1766 |      0 |    1766 |        0 |        0 |                  0 |           0 |
| libefuse.a                |       1616 |    64 |     4 |    60 |    10 |    10 |        0 |       1364 |   1364 |        178 |      0 |     178 |        0 |        0 |                  0 |           0 |
| libbrightness.a           |       1566 |     7 |     6 |     1 |     0 |     0 |        0 |       1530 |   1530 |         29 |      0 |      29 |        0 |        0 |                  0 |           0 |
| libesp_hal_wdt.a          |       1522 |     0 |     0 |     0 |  1522 |  1522 |        0 |          0 |      0 |          0 |      0 |       0 |        0 |        0 |                  0 |           0 |
| libsystem.a               |       1358 |     0 |     0 |     0 |     0 |     0 |        0 |       1333 |   1333 |         25 |      0 |      25 |        0 |        0 |                  0 |           0 |
| libwebsocket.a            |       1351 |    32 |    32 |     0 |     0 |     0 |        0 |       1304 |   1304 |         15 |      0 |      15 |        0 |        0 |                  0 |           0 |
| libesp_adc.a              |       1350 |    18 |    18 |     0 |     0 |     0 |        0 |       1253 |   1253 |         79 |      0 |      79 |        0 |        0 |                  0 |           0 |
| libi2c_bus.a              |       1317 |     4 |     4 |     0 |     0 |     0 |        0 |       1287 |   1287 |         26 |      0 |      26 |        0 |        0 |                  0 |           0 |
| librgb_led.a              |       1264 |     3 |     3 |     0 |     0 |     0 |        0 |       1211 |   1211 |         50 |      0 |      50 |        0 |        0 |                  0 |           0 |
| libpthread.a              |       1261 |    20 |     4 |    16 |     0 |     0 |        0 |       1152 |   1152 |         89 |      0 |      89 |        0 |        0 |                  0 |           0 |
| libesp_hal_uart.a         |       1079 |     0 |     0 |     0 |     0 |     0 |        0 |        935 |    935 |        144 |      0 |     144 |        0 |        0 |                  0 |           0 |
| libesp_hal_i2c.a          |       1072 |     0 |     0 |     0 |     0 |     0 |        0 |       1019 |   1019 |         53 |      0 |      53 |        0 |        0 |                  0 |           0 |
| libesp_hal_security.a     |       1069 |     0 |     0 |     0 |     0 |     0 |        0 |       1069 |   1069 |          0 |      0 |       0 |        0 |        0 |                  0 |           0 |
| libcalibration.a          |       1033 |     0 |     0 |     0 |     0 |     0 |        0 |       1003 |   1003 |         30 |      0 |      30 |        0 |        0 |                  0 |           0 |
| libtime_sync.a            |       1022 |     0 |     0 |     0 |     0 |     0 |        0 |        994 |    994 |         28 |      0 |      28 |        0 |        0 |                  0 |           0 |
| libmain.a                 |        763 |     0 |     0 |     0 |     0 |     0 |        0 |        729 |    729 |         34 |      0 |      34 |        0 |        0 |                  0 |           0 |
| libesp_coex.a             |        761 |    96 |     0 |    96 |   171 |   171 |        0 |        486 |    486 |          8 |      0 |       8 |        0 |        0 |                  0 |           0 |
| libsoc.a                  |        736 |    40 |     0 |    40 |    33 |    33 |        0 |         47 |     47 |        616 |      0 |     616 |        0 |        0 |                  0 |           0 |
| libesp_app_format.a       |        683 |    10 |    10 |     0 |     0 |     0 |        0 |        409 |    409 |        264 |      0 |       8 |      256 |        0 |                  0 |           0 |
| libesp_hal_clock.a        |        661 |     0 |     0 |     0 |     0 |     0 |        0 |        661 |    661 |          0 |      0 |       0 |        0 |        0 |                  0 |           0 |
| libcxx.a                  |        655 |    28 |    20 |     8 |     0 |     0 |        0 |        627 |    627 |          0 |      0 |       0 |        0 |        0 |                  0 |           0 |
| libesp_security.a         |        503 |     4 |     4 |     0 |     0 |     0 |        0 |        491 |    491 |          8 |      0 |       8 |        0 |        0 |                  0 |           0 |
| libesp_hal_ledc.a         |        502 |    24 |     0 |    24 |   388 |   388 |        0 |         22 |     22 |         68 |      0 |      68 |        0 |        0 |                  0 |           0 |
| libxt_hal.a               |        475 |     0 |     0 |     0 |   443 |   443 |        0 |          0 |      0 |         32 |      0 |      32 |        0 |        0 |                  0 |           0 |
| liblight_sensor.a         |        445 |     4 |     4 |     0 |     0 |     0 |        0 |        410 |    410 |         31 |      0 |      31 |        0 |        0 |                  0 |           0 |
| libcore.a                 |        327 |     9 |     9 |     0 |     0 |     0 |        0 |        275 |    275 |         43 |      0 |      43 |        0 |        0 |                  0 |           0 |
| libapp_update.a           |        227 |     4 |     4 |     0 |     0 |     0 |        0 |        193 |    193 |         30 |      0 |      30 |        0 |        0 |                  0 |           0 |
| libesp_hal_pmu.a          |        200 |     0 |     0 |     0 |     0 |     0 |        0 |        200 |    200 |          0 |      0 |       0 |        0 |        0 |                  0 |           0 |
| libgcc.a                  |        187 |     0 |     0 |     0 |     0 |     0 |        0 |        187 |    187 |          0 |      0 |       0 |        0 |        0 |                  0 |           0 |
| libinput.a                |         94 |     0 |     0 |     0 |     0 |     0 |        0 |         94 |     94 |          0 |      0 |       0 |        0 |        0 |                  0 |           0 |
| libnvs_sec_provider.a     |          5 |     0 |     0 |     0 |     0 |     0 |        0 |          5 |      5 |          0 |      0 |       0 |        0 |        0 |                  0 |           0 |
| libespnow.a               |          3 |     3 |     0 |     3 |     0 |     0 |        0 |          0 |      0 |          0 |      0 |       0 |        0 |        0 |                  0 |           0 |
+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

```

### Checkpoint results

Executing `mem` right after boot

```text
COS/flash> mem
Free heap:      18400 bytes
Min free heap:  14884 bytes
Total heap:     207144 bytes
Max alloc:      17408 bytes
Internal free:  18400 bytes
Internal min:   14884 bytes
Internal total: 207144 bytes
Internal max:   17408 bytes
```

Executing `berry profile` right after boot

```text
COS/flash> berry profile
R2 profile
  System heap:   free=18544 min=14884 total=207144 largest=17408
  Internal heap: free=18544 min=14884 total=207144 largest=17408
  Berry GC:      initialized=yes managed=3883 bytes
```

Executing `berry gc` right after boot

```text
COS/flash> berry gc
Berry GC: before=3883 after=3883 reclaimed=0 bytes
R2 profile
  System heap:   free=18544 min=14884 total=207144 largest=17408
  Internal heap: free=18544 min=14884 total=207144 largest=17408
  Berry GC:      initialized=yes managed=3883 bytes
```

### Regression results

(paste command outputs, please fill the table from them)

```text
COS/flash> berry run /flash/tests/r2/fs_text.be
fs_text sd ok true r2-text
next: berry profile
COS/flash> berry profile
R2 profile
  System heap:   free=17472 min=13128 total=207144 largest=15872
  Internal heap: free=17472 min=13128 total=207144 largest=15872
  Berry GC:      initialized=yes managed=4029 bytes
COS/flash>
```

```text
COS/flash> berry run /flash/tests/r2/fs_json.be
fs_json write true
fs_json ok true
fs_json value true 42 r2
next: berry profile
COS/flash> berry profile
R2 profile
  System heap:   free=26228 min=11908 total=207144 largest=15872
  Internal heap: free=26228 min=11908 total=207144 largest=15872
  Berry GC:      initialized=yes managed=4573 bytes
COS/flash>
```

```text
COS/flash> berry run /flash/tests/r2/http_get.be
http_get ok true status 200 err
http_get body_len 577
next: berry profile
COS/flash> berry profile
R2 profile
  System heap:   free=11972 min=7028 total=207144 largest=7168
  Internal heap: free=11972 min=7028 total=207144 largest=7168
  Berry GC:      initialized=yes managed=6390 bytes
```

```text
COS/flash> berry run /flash/tests/r2/https_get.be
E (147248) esp-tls-mbedtls: mbedtls_ssl_setup returned -0x008D
E (147258) esp-tls: create_ssl_handle failed
E (147308) esp-tls: Failed to open new connection
E (147308) transport_base: Failed to open a new connection
E (147308) HTTP_CLIENT: Connection failed, sock < 0
https_get ok false status 0 err ESP_ERR_HTTP_CONNECT
next: berry profile
COS/flash> berry profile
R2 profile
  System heap:   free=12700 min=2604 total=207144 largest=4096
  Internal heap: free=12700 min=2604 total=207144 largest=4096
  Berry GC:      initialized=yes managed=5188 bytes
```

```text
COS/flash> berry run /flash/tests/r2/json_fetch.be
json_fetch ok false status 0 json_ok false err ESP_ERR_INVALID_STATE
next: berry profile
COS/flash> berry profile
R2 profile
  System heap:   free=5284 min=4460 total=207144 largest=3968
  Internal heap: free=5284 min=4460 total=207144 largest=3968
  Berry GC:      initialized=yes managed=4434 bytes
COS/flash>
```

```text
berry exec /flash/tests/r2/repeated_fetch.be https://google.com 5 "text"
E (132598) esp-tls-mbedtls: mbedtls_ssl_setup returned -0x008D
E (132598) esp-tls: create_ssl_handle failed
E (132598) esp-tls: Failed to open new connection
E (132598) transport_base: Failed to open a new connection
E (132668) HTTP_CLIENT: Connection failed, sock < 0
fetch 1 / 5 ok false status 0 err ESP_ERR_HTTP_CONNECT
E (132748) esp-tls-mbedtls: mbedtls_ssl_setup returned -0x008D
E (132758) esp-tls: create_ssl_handle failed
E (132758) esp-tls: Failed to open new connection
E (132818) transport_base: Failed to open a new connection
E (132838) HTTP_CLIENT: Connection failed, sock < 0
fetch 2 / 5 ok false status 0 err ESP_ERR_HTTP_CONNECT
E (132948) esp-tls-mbedtls: mbedtls_ssl_setup returned -0x008D
E (132958) esp-tls: create_ssl_handle failed
E (132958) esp-tls: Failed to open new connection
E (133018) transport_base: Failed to open a new connection
E (133018) HTTP_CLIENT: Connection failed, sock < 0
fetch 3 / 5 ok false status 0 err ESP_ERR_HTTP_CONNECT
E (133108) esp-tls-mbedtls: mbedtls_ssl_setup returned -0x008D
E (133128) esp-tls: create_ssl_handle failed
E (133128) esp-tls: Failed to open new connection
E (133138) transport_base: Failed to open a new connection
E (133138) HTTP_CLIENT: Connection failed, sock < 0
fetch 4 / 5 ok false status 0 err ESP_ERR_HTTP_CONNECT
E (133258) esp-tls-mbedtls: mbedtls_ssl_setup returned -0x008D
E (133268) esp-tls: create_ssl_handle failed
E (133268) esp-tls: Failed to open new connection
E (133328) transport_base: Failed to open a new connection
E (133328) HTTP_CLIENT: Connection failed, sock < 0
fetch 5 / 5 ok false status 0 err ESP_ERR_HTTP_CONNECT
repeated_fetch done 0 / 5
next: berry profile; berry gc; berry profile
COS/flash> berry profile
R2 profile
  System heap:   free=8768 min=668 total=207144 largest=3328
  Internal heap: free=8768 min=668 total=207144 largest=3328
  Berry GC:      initialized=yes managed=8839 bytes
COS/flash> berry gc
Berry GC: before=8839 after=5224 reclaimed=3615 bytes
R2 profile
  System heap:   free=12100 min=668 total=207144 largest=3328
  Internal heap: free=12116 min=668 total=207144 largest=3328
  Berry GC:      initialized=yes managed=5224 bytes
COS/flash> berry profile
R2 profile
  System heap:   free=12108 min=668 total=207144 largest=3328
  Internal heap: free=12108 min=668 total=207144 largest=3328
  Berry GC:      initialized=yes managed=5224 bytes
```

### Anomalies / follow-ups

- **Critical: native heap exhaustion/fragmentation on network timeout/error paths.** Fresh-boot reruns B/C/D reached `min=52–72` bytes and `largest=384–1344` bytes; `berry gc` reclaimed `0` Berry-managed bytes while GC managed stayed ~4428. The dominant issue is native heap/fragmentation, not Berry garbage.
- **HTTPS fails on clean boot due to TLS allocation pressure.** Run A (network confirmed by `ping example.com`): `transport_base: Failed to initialize new connection object` / `ESP_ERR_HTTP_CONNECT`; post-failure profile showed `free=13984 min=1148 largest=8192`, GC 4434, no GC reclamation. Earlier isolated attempt showed `mbedtls_ssl_setup returned -0x008D` (`MBEDTLS_ERR_SSL_ALLOC_FAILED`). HTTPS cannot be considered working/cert-verified until a clean-boot success is measured.
- **HTTP success path is functional but expensive.** Clean-boot HTTP success (158-byte body, status 200): `free=13332 min=3288 largest=6912`, GC 4728, `berry gc` reclaimed `0`. Largest block/min watermark leave little margin for TLS.
- **BT coexistence appears in failure signatures.** `BT_OSI: malloc failed (btc_transfer_context, size=536)` and `heap info: free~1100-1684, largest_block~320` were logged during collapses; BLE HID open failures also appeared. BT may be a victim of pressure and/or a contributor; measure Wi-Fi+BT+HTTP/TLS together.
- **Timeout handling is operationally dangerous.** After caller timeout, async `esp-tls select() timeout` / `transport_base` / `HTTP_CLIENT` errors appear later; in some cases autoexec or later Berry execution fails (`ESP_ERR_TIMEOUT`, `ESP_FAIL`, `ESP_ERR_NO_MEM` from HTTP client). The worker abandons caller-visible completion but the underlying client/transport may still be consuming memory/time until it unwinds.
- **Rerun D was not a valid “successful repeated HTTP” stability test.** `http://127.0.0.1` caused DNS/connection failure then repeated `ESP_ERR_NO_MEM` in esp_http_client; it demonstrates collapse under error pressure, not steady-state HTTP stability. Repeat D with a known-good HTTP 200 endpoint.
- **Missing/invalid measurements:** rerun step 3 in the original plan was confounded (ran after heap collapse); rerun A is now isolated but should include exact `https_get.be` URL. Record script URLs/config on device for all runs.

## Rerun Plan results (after restart and clean boot)

1 and 2.

```text
COS/flash> mem
Free heap:      18484 bytes
Min free heap:  13440 bytes
Total heap:     207144 bytes
Max alloc:      17408 bytes
Internal free:  18484 bytes
Internal min:   13440 bytes
Internal total: 207144 bytes
Internal max:   17408 bytes
COS/flash> berry profile
R2 profile
  System heap:   free=18484 min=13440 total=207144 largest=17408
  Internal heap: free=18484 min=13440 total=207144 largest=17408
  Berry GC:      initialized=yes managed=3883 bytes
COS/flash> berry run /flash/tests/r2/http_get.be
E (31188) BT_OSI: malloc failed (caller=0x40109ea8 size=536)
--- 0x40109ea8: btc_transfer_context at C:/esp/v6.1/esp-idf/components/bt/common/btc/core/btc_task.c:348
E (31188) BT_OSI: heap info: free=1100, largest_block=320
E (31198) BT_BTC: btc_gattc_cback transfer failed

http_get ok false status 0 err ESP_ERR_TIMEOUT
next: berry profile
COS/flash> E (31868) esp-tls: [sock=57] select() timeout
E (31868) transport_base: Failed to open a new connection: 32774

COS/flash> berry run /flash/tests/r2/http_get.be
berry: execution failed (ESP_FAIL)

COS/flash> berry profile
R2 profile
  System heap:   free=5300 min=76 total=207144 largest=1792
  Internal heap: free=5300 min=76 total=207144 largest=1792
  Berry GC:      initialized=yes managed=4428 bytes
COS/flash> berry gc
Berry GC: before=4428 after=4428 reclaimed=0 bytes
R2 profile
  System heap:   free=5300 min=76 total=207144 largest=1792
  Internal heap: free=5300 min=76 total=207144 largest=1792
  Berry GC:      initialized=yes managed=4428 bytes
COS/flash> berry profile
R2 profile
  System heap:   free=5300 min=76 total=207144 largest=1792
  Internal heap: free=5300 min=76 total=207144 largest=1792
  Berry GC:      initialized=yes managed=4428 bytes
COS/flash>
```

3.

```text
COS/flash> berry run /flash/tests/r2/https_get.be
E (40108) esp-tls-mbedtls: mbedtls_ssl_setup returned -0x008D
E (40108) esp-tls: create_ssl_handle failed
E (40118) esp-tls: Failed to open new connection
E (40118) transport_base: Failed to open a new connection
E (40128) HTTP_CLIENT: Connection failed, sock < 0
https_get ok false status 0 err ESP_ERR_HTTP_CONNECT
next: berry profile
COS/flash> berry profile
R2 profile
  System heap:   free=13752 min=5524 total=207144 largest=7168
  Internal heap: free=13752 min=5524 total=207144 largest=7168
  Berry GC:      initialized=yes managed=4434 bytes
COS/flash> berry gc
Berry GC: before=4434 after=4434 reclaimed=0 bytes
R2 profile
  System heap:   free=13752 min=5524 total=207144 largest=7168
  Internal heap: free=13752 min=5524 total=207144 largest=7168
  Berry GC:      initialized=yes managed=4434 bytes
COS/flash> berry profile
R2 profile
  System heap:   free=13752 min=5524 total=207144 largest=7168
  Internal heap: free=13752 min=5524 total=207144 largest=7168
  Berry GC:      initialized=yes managed=4434 bytes
```

4. (after fresh boot)

```text
COS/flash> berry run /flash/tests/r2/json_fetch.be
E (32178) BT_OSI: malloc failed (caller=0x40109ea8 size=536)
--- 0x40109ea8: btc_transfer_context at C:/esp/v6.1/esp-idf/components/bt/common/btc/core/btc_task.c:348
E (32178) BT_OSI: heap info: free=1632, largest_block=320
E (32178) BT_BTC: btc_gattc_cback transfer failed

E (32178) BT_OSI: malloc failed (caller=0x40109ea8 size=536)
--- 0x40109ea8: btc_transfer_context at C:/esp/v6.1/esp-idf/components/bt/common/btc/core/btc_task.c:348
E (32198) BT_OSI: heap info: free=1660, largest_block=320
E (32198) BT_BTC: btc_gattc_cback transfer failed

json_fetch ok false status 0 json_ok false err ESP_ERR_TIMEOUT
next: berry profile
COS/flash> E (33238) esp-tls: [sock=57] select() timeout
E (33238) transport_base: Failed to open a new connection: 32774
E (33238) HTTP_CLIENT: Connection failed, sock < 0
berry profile
R2 profile
  System heap:   free=5332 min=52 total=207144 largest=1152
  Internal heap: free=5332 min=52 total=207144 largest=1152
  Berry GC:      initialized=yes managed=4428 bytes
COS/flash> berry gc
Berry GC: before=4428 after=4428 reclaimed=0 bytes
R2 profile
  System heap:   free=5332 min=52 total=207144 largest=1152
  Internal heap: free=5332 min=52 total=207144 largest=1152
  Berry GC:      initialized=yes managed=4428 bytes
COS/flash> berry profile
R2 profile
  System heap:   free=5332 min=52 total=207144 largest=1152
  Internal heap: free=5332 min=52 total=207144 largest=1152
  Berry GC:      initialized=yes managed=4428 bytes
COS/flash>
```

D (as reported; not a valid "successful repeated HTTP" stability test — used `http://127.0.0.1`, then esp_http_client hit `ESP_ERR_NO_MEM`/allocation failures; demonstrates collapse under error pressure):

```text
COS/flash> E (2058) shell_berry: Could not run /flash/autoexec.be: ESP_ERR_TIMEOUT
berry profile
R2 profile
  System heap:   free=18768 min=15324 total=207144 largest=18432
  Internal heap: free=18768 min=15324 total=207144 largest=18432
  Berry GC:      initialized=yes managed=3899 bytes
COS/flash> berry exec /flash/tests/r2/repeated_fetch.be http://127.0.0.1 10 text
E (19658) esp-tls: couldn't get hostname for :127.0.0.1: getaddrinfo() returns 203, addrinfo=0
E (19658) transport_base: Failed to open a new connection: 32769
E (19658) HTTP_CLIENT: Connection failed, sock < 0
fetch 1 / 10 ok false status 0 err ESP_ERR_HTTP_CONNECT
E (19688) HTTP_CLIENT: Failed to allocate memory for host header
fetch 2 / 10 ok false status 0 err ESP_ERR_NO_MEM
E (19708) HTTP_CLIENT: Allocation failed
fetch 3 / 10 ok false status 0 err ESP_ERR_NO_MEM
E (19728) HTTP_CLIENT: Allocation failed
fetch 4 / 10 ok false status 0 err ESP_ERR_NO_MEM
E (19738) HTTP_CLIENT: Allocation failed
fetch 5 / 10 ok false status 0 err ESP_ERR_NO_MEM
E (19748) HTTP_CLIENT: Allocation failed
fetch 6 / 10 ok false status 0 err ESP_ERR_NO_MEM
E (19758) HTTP_CLIENT: Allocation failed
fetch 7 / 10 ok false status 0 err ESP_ERR_NO_MEM
E (19768) HTTP_CLIENT: Allocation failed
fetch 8 / 10 ok false status 0 err ESP_ERR_NO_MEM
E (19828) HTTP_CLIENT: Allocation failed
fetch 9 / 10 ok false status 0 err ESP_ERR_NO_MEM
E (19838) HTTP_CLIENT: Allocation failed
fetch 10 / 10 ok false status 0 err ESP_ERR_NO_MEM
repeated_fetch done 0 / 10
next: berry profile; berry gc; berry profile
COS/flash> berry profile
R2 profile
  System heap:   free=1680 min=24 total=207144 largest=736
  Internal heap: free=1680 min=24 total=207144 largest=736
  Berry GC:      initialized=yes managed=7328 bytes
COS/flash> berry gc
Berry GC: before=7328 after=3960 reclaimed=3368 bytes
R2 profile
  System heap:   free=5656 min=24 total=207144 largest=1920
  Internal heap: free=5656 min=24 total=207144 largest=1920
  Berry GC:      initialized=yes managed=3960 bytes
COS/flash> berry profile
R2 profile
  System heap:   free=5664 min=24 total=207144 largest=1920
  Internal heap: free=5664 min=24 total=207144 largest=1920
  Berry GC:      initialized=yes managed=3960 bytes
COS/flash>
```

## R8/P4 step 1 instrumentation (implemented 2026-11-08)

Code changes (no stack-size tuning):
- `components/network/src/network_http.c`: logs after each HTTP worker request: `R8/P4 stack: cfg=<bytes> unit=<sizeof(StackType_t)> hwm_words=<n> hwm_bytes=<n> used~=<bytes> min_free=<hwm_bytes> method=<...> scheme=<http|https> status=<n> err=<esp_err>`.
- `components/network/include/network.h` + `network_http.c`: `network_http_worker_stack_info_t` / `network_http_get_worker_stack_info()` expose the last sample.
- `components/shell/src/shell_berry_cmd.c`: `berry profile` prints `HTTP worker: ...` (configured units when not started; last sample when available).

Capture on hardware: run an HTTP/HTTPS/timeout/error request, then copy the `net_http: R8/P4 stack:` serial line and run `berry profile`. Record HWM for GET, POST/body (if tested), HTTPS, max response, timeout, and error paths before any `HTTP_TASK_STACK` change.

## Analysis and improvement opportunities (2026-11-08)

### Evidence summary

- **Boot is tight but usable:** ~18.4–18.6 KB free, ~13.4–16.2 KB min, 13.3–18.4 KB largest (varies by boot/BT state), Berry GC ~3883–3899 bytes.
- **HTTP success (158 B, clean boot):** functional, but `min=3288`, `largest=6912` after; `berry gc` reclaimed 0. Native peak/fragmentation dominates.
- **HTTPS (clean boot, network OK by ping):** TLS setup/connect fails (`MBEDTLS_ERR_SSL_ALLOC_FAILED -0x008D` in earlier isolated run; `Failed to initialize new connection object` in run A). Run A post-failure: `min=1148`, `largest=8192`, no GC reclamation.
- **Timeout/error paths:** can collapse to `min=24–76`, `largest=384–1920`, with `ESP_ERR_TIMEOUT`, late async `esp-tls select() timeout`/transport errors, BT malloc failures, and esp_http_client `ESP_ERR_NO_MEM`/allocation failures. Berry GC does not recover native heap.
- **Static footprint (current build):** `libnetwork.a` DRAM/.bss **15802** bytes; `libberry_lang.a` **5983**; total DRAM 81.74% used. HTTP worker stack is `HTTP_TASK_STACK 12288` in `components/network/src/network_http.c` (allocated on first HTTP request); confirm ESP-IDF stack units before comparing to bytes.

### What can still be improved

Prioritized by expected impact and risk. Measure each change with the same P7 metrics (`free`, `min`, `largest`, internal variants, Berry GC) from a clean boot.

| Priority | Improvement | Where | Rationale / evidence | Risk / notes |
|---|---|---|---|---|
| P0 | **Measure and cap HTTP worker stack (R8/P4)** | `components/network/src/network_http.c` (`HTTP_TASK_STACK 12288`) | 12 KB is allocated for the persistent `net_http` task on first fetch. On this DRAM-constrained target, that can dominate heap. Add `uxTaskGetStackHighWaterMark()` logging under GET/POST/HTTP/HTTPS/timeout/error paths; account for ESP-IDF stack units; reduce only with measured peak + >=25% margin. | Medium: too small causes hard-to-debug stack overflow. |
| P0 | **Reduce mbedTLS/TLS peak memory (R8/P5)** | `sdkconfig.defaults` / mbedTLS config | HTTPS fails with `-0x008D` from clean boot; HTTP success leaves only 6912 largest / 3288 min. Evaluate `CONFIG_MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH`, `CONFIG_MBEDTLS_DYNAMIC_BUFFER`/`DYNAMIC_FREE_CONFIG_DATA`, and smaller `MBEDTLS_SSL_IN_CONTENT_LEN`/`OUT_CONTENT_LEN` (compatibility-tested). Keep `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y` and verification enabled. | Medium-High: can affect TLS compatibility/performance; test against target HTTPS endpoints. |
| P0 | **Fix/understand timeout+cleanup lifecycle** | `components/network/src/network_http.c`, `network_http_fetch`/worker | Caller returns `ESP_ERR_TIMEOUT` and marks request abandoned, but esp_http_client/transport may continue until internal timeout; async `select()`/transport errors appear later. During these paths, heap reached min 24–76 and caused BT/HTTP client alloc failures. Ensure all error paths clean up promptly; consider whether connect/read timeouts can be bounded more tightly and whether abandoned operations should avoid extra allocations/log noise. Do not introduce races: worker still owns request/response buffers while active. | High: concurrency/lifetime bugs can cause use-after-free. |
| P1 | **Add HTTP worker idle shutdown (P4 optional, only if race-free)** | `network_http.c` | Persistent worker holds stack forever after first fetch. If measured as beneficial, delete task after an idle period and recreate on demand. | Medium: queue/semaphore/task lifecycle races. |
| P1 | **Reduce static response/header footprint if limits allow** | `components/network/include/network.h`, `network_http.c` | `libnetwork.a` has 15802 B `.bss`. `s_response` includes URL + 12 response headers with fixed 41+257-byte fields; request headers 8×; request/response bodies 4 KB each. Review whether `NETWORK_HTTP_RESPONSE_HEADER_MAX`, `NETWORK_HTTP_HEADER_VALUE_MAX`, or body max can be safely reduced/profiled; prefer measurement over arbitrary cuts (P6/P1 guardrails). | Medium: API/behavior limits; may break scripts expecting larger headers/bodies. |
| P1 | **Investigate BT + Wi-Fi coexistence memory** | `sdkconfig.defaults` (`CONFIG_BT_ENABLED`, BLE/GATTC, Wi-Fi IRAM opts), runtime feature use | BT malloc failures (`btc_transfer_context` 536 B) occurred when heap was already collapsing. Determine whether BT can be disabled/released in configurations that don’t need it, or whether BLE HID/GATT usage can be narrowed. Any change must preserve required product behavior. | Medium/High: product/config tradeoff. |
| P1 | **Make HTTP/TLS failure reporting actionable and bounded** | `network_http.c`, Berry `net.fetch`/scripts | Repeated error-path attempts produced `ESP_ERR_NO_MEM` inside esp_http_client and catastrophic fragmentation. Consider failing fast when preconditions are bad, limiting retries in scripts, and returning clear errors without repeated expensive setup when heap/largest block is already unsafe (careful heuristic only if measured reliable). | Medium: avoid false negatives. |
| P2 | **Validate “repeated successful HTTP” stability** | Hardware procedure; `repeated_fetch.be` target | Rerun D used `127.0.0.1` and is invalid for stability. Re-run from clean boot against a reliable HTTP 200 endpoint (10+ requests), then `berry profile`/`berry gc`/`berry profile`. | Low risk, required evidence. |
| P2 | **Isolate HTTPS success conditions** | Test procedure, then P5 config/code | Use a known-good HTTPS endpoint with a cert in the configured bundle, from clean boot, before any degrading test. Capture before/after/GC profiles. If still `-0x008D`, focus on mbedTLS buffer/content length and peak heap. | Low risk for test; config changes need care. |

### Suggested next implementation order (after R2 evidence)

1. **R8/P4 measurement first:** stack HWM + TLS/HTTP peak/largest-block timeline (no blind tuning).
2. **R8/P5 mbedTLS memory config experiment:** minimal compatible reduction, verify HTTPS + cert verification.
3. **Timeout/abandon cleanup audit:** remove avoidable post-timeout work and confirm cleanup on every path.
4. **Re-profile:** clean-boot HTTP success, HTTPS success/failure, timeout, repeated HTTP success. Update this report and R2/R8 progress.
