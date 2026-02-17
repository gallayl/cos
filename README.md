# COS

COS is a firmware project for the ESP32-based CYD board (ESP32-2432S028R), featuring an ILI9341 display with XPT2046 resistive touch.

## Prerequisites

### ESP-IDF

Install [ESP-IDF v5.5.2](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32/get-started/index.html) for your platform.

### Environment variables

The Makefile requires two environment variables to locate the ESP-IDF toolchain:

| Variable | Example (Windows) | Example (Linux / devcontainer) |
|---|---|---|
| `IDF_PATH` | `C:\esp\v5.5.2\esp-idf` | `/opt/esp/idf` |
| `IDF_TOOLS_PATH` | `C:\Espressif\tools` | `/opt/esp` |
| `IDF_PYTHON_ENV_PATH` | `C:\Espressif\tools\python\v5.5.2\venv` | `/opt/esp/python_env/idf5.5_py3.x_env` |

**Windows (CMD):**

```cmd
set IDF_PATH=C:\esp\v5.5.2\esp-idf
set IDF_TOOLS_PATH=C:\Espressif\tools
set IDF_PYTHON_ENV_PATH=C:\Espressif\tools\python\v5.5.2\venv
```

**Windows (PowerShell):**

```powershell
$env:IDF_PATH = "C:\esp\v5.5.2\esp-idf"
$env:IDF_TOOLS_PATH = "C:\Espressif\tools"
$env:IDF_PYTHON_ENV_PATH = "C:\Espressif\tools\python\v5.5.2\venv"
```

**Linux / macOS:**

```bash
export IDF_PATH=/opt/esp/idf
export IDF_TOOLS_PATH=/opt/esp
export IDF_PYTHON_ENV_PATH=/opt/esp/python_env/idf5.5_py3.x_env
```

### Host tools

| Tool | Purpose | Install |
|---|---|---|
| GNU Make | Build orchestration | Included with MSYS2 / Xcode CLT / apt |
| CMake | Test build system | Bundled with ESP-IDF (`C:\Espressif\tools\cmake`) |
| GCC (host) | Compile host-based unit tests | `winget install BrechtSanders.WinLibs.POSIX.UCRT` (Windows) or `apt install gcc` (Linux) |
| clang-format | Code formatting | Bundled with LLVM or ESP-IDF clang toolchain |
| clang-tidy | Static analysis | Bundled with LLVM or ESP-IDF clang toolchain |

## Build and flash

```bash
make build       # compile the firmware
make flash       # flash to the connected board
make monitor     # open serial monitor (115200 baud)
```

## Console

Once flashed, the firmware starts an interactive console on UART (`cos>` prompt). Available commands:

| Command | Description |
|---|---|
| `calibrate` | Run interactive touch screen calibration |
| `help` | List all registered commands |

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
