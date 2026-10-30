# COS

COS is a firmware project for the ESP32-based CYD board (ESP32-2432S028R), featuring an ILI9341 display with XPT2046 resistive touch.

## Prerequisites

### ESP-IDF

Install [ESP-IDF v5.5.2](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32/get-started/index.html) for your platform.

### Environment variables

The Makefile requires two environment variables to locate the ESP-IDF toolchain:

| Variable              | Example (Windows)                     | Example (Linux / devcontainer)         |
| --------------------- | ------------------------------------- | -------------------------------------- |
| `IDF_PATH`            | `c:\esp\v6.1\esp-idf\`                | `/opt/esp/idf`                         |
| `IDF_TOOLS_PATH`      | `C:\Espressif\tools`                  | `/opt/esp`                             |
| `IDF_PYTHON_ENV_PATH` | `C:\Espressif\tools\python\v6.1\venv` | `/opt/esp/python_env/idf5.5_py3.x_env` |

**Windows (CMD):**

```cmd
set ESP_IDF_VERSION=6.1
set IDF_PATH=c:/esp/v6.1/esp-idf/
set IDF_TOOLS_PATH=C:/Espressif/tools
set IDF_PYTHON_ENV_PATH=C:/Espressif/tools/python/v6.1/venv
```

**Windows (PowerShell):**

```powershell
$env:ESP_IDF_VERSION = "6.1"
$env:IDF_PATH = "c:/esp/v6.1/esp-idf/"
$env:IDF_TOOLS_PATH = "C:/Espressif/tools"
$env:IDF_PYTHON_ENV_PATH = "C:/Espressif/tools/python/v6.1/venv"
```

**Linux / macOS:**

```bash
export ESP_IDF_VERSION=6.1
export IDF_PATH=~/.espressif/v6.1/esp-idf
export IDF_TOOLS_PATH=~/.espressif/tools
export IDF_PYTHON_ENV_PATH=~/.espressif/python_env/idf5.5_py3.14_env
```

### Host tools

| Tool         | Purpose                       | Install                                                                                  |
| ------------ | ----------------------------- | ---------------------------------------------------------------------------------------- |
| GNU Make     | Build orchestration           | Included with MSYS2 / Xcode CLT / apt                                                    |
| CMake        | Test build system             | Bundled with ESP-IDF (`C:\Espressif\tools\cmake`)                                        |
| GCC (host)   | Compile host-based unit tests | `winget install BrechtSanders.WinLibs.POSIX.UCRT` (Windows) or `apt install gcc` (Linux) |
| clang-format | Code formatting               | Bundled with LLVM or ESP-IDF clang toolchain                                             |
| clang-tidy   | Static analysis               | Bundled with LLVM or ESP-IDF clang toolchain                                             |

## Build and flash

```bash
make build       # compile the firmware
make flash       # flash to the connected board
make flash-game  # build and flash the example Berry game to LittleFS only
make monitor     # open serial monitor (115200 baud)
```

`make flash-game` packages `examples/flash_data` as a LittleFS image and flashes only the `littlefs` partition; it does not flash firmware or erase other partitions. It leaves the board in the bootloader after transfer; press the board's reset button to boot COS. To select a serial port, use `make flash-game PORT=COM5` (replace with your port). After flashing, start either Berry game from the COS console:

```text
berry run /flash/apps/breakout.be
```

Drag your finger across the screen to move the Breakout paddle. Tap **RESET** to restart or **EXIT** to return to the console. The Tap Dash example is also available with `berry run /flash/apps/tap_dash.be`.

## Console

Once flashed, the firmware starts an interactive console on UART (`cos>` prompt). Available commands:

| Command     | Description                              |
| ----------- | ---------------------------------------- |
| `calibrate` | Run interactive touch screen calibration |
| `berry run <path>` | Execute a Berry script from `/flash` or `/sdcard` |
| `berry eval <source>` | Evaluate Berry source in the shared VM |
| `berry close` | Close the active Berry app and run its teardown method |
| `help`      | List all registered commands             |

Berry scripts use COS virtual paths. For example, `berry run /flash/apps/demo.be` runs a flash script, and `berry run /sdcard/apps/demo.be` runs one from an inserted SD card. Relative paths are resolved from the shell's current directory. Use `berry eval 'print(1 + 2)'` for a short expression. The runtime exposes `log(message)` and `action(command)`; `action` runs a registered COS console command and returns its integer command status.

The Berry interpreter is fetched from a fixed upstream commit during CMake configuration. Python 3 is required to generate Berry's constant tables during the build.

Class-based Berry files returned from `berry run` are instantiated as apps and receive `setup(content, width, height)`; `berry close` invokes the optional `teardown()` method. The migrated Berry `ui` module currently supports labels, buttons, group boxes, checkboxes, bounds, text updates/colors, click/change handlers, timers, random values, deferred app exit, and clearing containers. Other cyd-proto widgets and the desktop/window framework are still migration work in progress.

While a Berry UI app is active, its UI owns the display and the text-console renderer is paused to prevent the console and UI from drawing over each other. The UI redraws only when its content changes, and console output continues over UART and is rendered on screen again after `berry close` or `ui.exit()`. Berry game apps can also use `ui.fill_screen`, `ui.fill_rect`, and `ui.draw_text` for direct display drawing, plus `ui.touch()` to read the current touch position. Use `ui.start_write()` and `ui.end_write()` to batch related display operations into one write transaction.

## Development

```bash
make format       # auto-format all source files
make format-check # check formatting (CI-friendly, fails on violations)
make lint         # run clang-tidy static analysis
make test         # build and run host-based unit tests
make menuconfig   # open ESP-IDF configuration menu
make clean        # remove build artifacts
make fullclean    # remove build artifacts and sdkconfig
```

## Project structure

```
cos/
  CMakeLists.txt              # ESP-IDF root project file
  Makefile                    # Build orchestration
  sdkconfig.defaults          # Non-default ESP-IDF settings (tracked)
  main/
    main.c                    # Entry point: NVS, display, calibration, console init
  components/
    berry_lang/             # Berry VM and virtual-filesystem script runner
      include/berry_runtime.h # Berry lifecycle and execution API
    ui/                     # Renderer-independent touch UI primitives
      include/ui.h          # Elements, containers, labels, buttons, group boxes, checkboxes, touch dispatch
    display/                  # LovyanGFX display + touch driver
      include/display.h       # Public C API
      src/display.cpp         # LovyanGFX wrapper
      src/cyd_board_config.h  # CYD pin definitions
      idf_component.yml       # LovyanGFX dependency
    calibration/              # Touch calibration with NVS storage
      include/calibration.h   # Public C API
      src/calibration.c       # Orchestration (init, load, run)
      src/calibration_storage.c/h  # NVS read/write per rotation
      src/calibration_cmd.c   # esp_console "calibrate" command
  test/                       # Host-based unit tests (CMake + Unity)
    mocks/                    # Fakes for NVS, display, ESP-IDF headers
    test_calibration_storage.c
    test_calibration.c
```
