.PHONY: build flash flash-data monitor clean fullclean erase-flash format format-check lint test integration-test integration-test-build integration-test-run menuconfig docs docs-check

# --- ESP-IDF environment setup (portable: Windows + Linux) ---

# Add ESP-IDF tools (cmake, ninja, compilers) to PATH automatically
ifdef IDF_TOOLS_PATH
  IDF_TOOL_DIRS := $(wildcard $(IDF_TOOLS_PATH)/cmake/*/bin) \
                   $(wildcard $(IDF_TOOLS_PATH)/ninja/*) \
                   $(wildcard $(IDF_TOOLS_PATH)/xtensa-esp-elf/*/xtensa-esp-elf/bin) \
                   $(wildcard $(IDF_TOOLS_PATH)/xtensa-esp-elf-gdb/*/xtensa-esp-elf-gdb/bin)
  SEP := $(if $(findstring ;,$(PATH)),;,:)
  EXTRA_PATH := $(subst $(eval ) ,$(SEP),$(IDF_TOOL_DIRS))
  export PATH := $(EXTRA_PATH)$(SEP)$(PATH)
endif

# Use IDF_PYTHON_ENV_PATH for the correct venv python, fall back to system python
ifdef IDF_PYTHON_ENV_PATH
  ifneq ($(wildcard $(IDF_PYTHON_ENV_PATH)/Scripts/python.exe),)
    IDF_PYTHON ?= $(IDF_PYTHON_ENV_PATH)/Scripts/python.exe
  else
    IDF_PYTHON ?= $(IDF_PYTHON_ENV_PATH)/bin/python
  endif
else
  IDF_PYTHON ?= python
endif
IDF_PY ?= $(IDF_PYTHON) $(IDF_PATH)/tools/idf.py
export IDF_PYTHON
export IDF_PY

# Portable source file listing (works with both GNU and Windows find)
SOURCES := $(wildcard main/*.c main/*.h) \
           $(wildcard components/display/include/*.h) \
           $(wildcard components/display/src/*.cpp components/display/src/*.h) \
           $(wildcard components/calibration/include/*.h) \
           $(wildcard components/calibration/src/*.c components/calibration/src/*.h) \
           $(wildcard components/rgb_led/include/*.h) \
           $(wildcard components/rgb_led/src/*.c components/rgb_led/src/*.h) \
           $(wildcard components/light_sensor/include/*.h) \
           $(wildcard components/light_sensor/src/*.c components/light_sensor/src/*.h) \
           $(wildcard components/brightness/include/*.h) \
           $(wildcard components/brightness/src/*.c components/brightness/src/*.h) \
           $(wildcard components/filesystem/include/*.h) \
           $(wildcard components/filesystem/src/*.c components/filesystem/src/*.h) \
           $(wildcard components/shell/include/*.h) \
           $(wildcard components/shell/src/*.c components/shell/src/*.h) \
           $(wildcard components/wifi/include/*.h) \
           $(wildcard components/wifi/src/*.c components/wifi/src/*.h) \
           $(wildcard components/bluetooth/include/*.h) \
           $(wildcard components/bluetooth/src/*.c components/bluetooth/src/*.h) \
           $(wildcard components/input/include/*.h) \
           $(wildcard components/input/src/*.c components/input/src/*.h) \
           $(wildcard components/network/include/*.h) \
           $(wildcard components/network/src/*.c components/network/src/*.h) \
           $(wildcard components/i2c_bus/include/*.h) \
           $(wildcard components/i2c_bus/src/*.c components/i2c_bus/src/*.h) \
           $(wildcard components/time_sync/include/*.h) \
           $(wildcard components/time_sync/src/*.c components/time_sync/src/*.h) \
           $(wildcard components/system/include/*.h) \
           $(wildcard components/system/src/*.c components/system/src/*.h) \
           $(wildcard components/http_server/include/*.h) \
           $(wildcard components/http_server/src/*.c components/http_server/src/*.h) \
           $(wildcard components/websocket/include/*.h) \
           $(wildcard components/websocket/src/*.c components/websocket/src/*.h) \
           $(wildcard components/text_console/include/*.h) \
           $(wildcard components/text_console/src/*.cpp components/text_console/src/*.c components/text_console/src/*.h) \
           $(wildcard components/berry_lang/include/*.h) \
           $(wildcard components/berry_lang/src/*.cpp components/berry_lang/src/*.c components/berry_lang/src/*.h) \
           $(wildcard components/ui/include/*.h) \
           $(wildcard components/ui/src/*.cpp components/ui/src/*.h) \
           $(wildcard components/shell/src/shell_berry_cmd.c)

INTEGRATION_TEST_PROJECTS := $(patsubst %/,%,$(wildcard integration_test/test_*/))

build:
	$(IDF_PY) build

flash:
	$(IDF_PY) flash

flash-data: build
	cd "$(CURDIR)/build" && $(IDF_PYTHON) -m esptool --chip esp32 $(if $(PORT),-p $(PORT),) write-flash @littlefs-flash_args

erase-flash:
	$(IDF_PY) erase-flash

monitor:
	$(IDF_PY) monitor

clean:
	$(IDF_PY) clean

fullclean:
	$(IDF_PY) fullclean

menuconfig:
	$(IDF_PY) menuconfig

format:
	clang-format -i $(SOURCES)

format-check:
	clang-format --dry-run --Werror $(SOURCES)

lint:
	@$(IDF_PYTHON) scripts/run_tidy.py $(SOURCES)

test:
	cmake -G Ninja -DCMAKE_C_COMPILER=gcc -B test/build test
	cmake --build test/build
	ctest --test-dir test/build --output-on-failure
	python -m unittest discover -s scripts -p "test_*.py"

integration-test-install:
	$(IDF_PYTHON) $(IDF_PATH)/tools/idf_tools.py install qemu-xtensa

integration-test-build:
	$(IDF_PYTHON) scripts/integration_test.py build $(INTEGRATION_TEST_BUILD_ARGS)

integration-test-run:
	$(IDF_PYTHON) scripts/integration_test.py run $(INTEGRATION_TEST_RUN_ARGS)

integration-test: integration-test-build integration-test-run

docs:
	@cmake -E make_directory build/docs
	( cat Doxyfile ; echo "WARN_AS_ERROR = NO" ) | doxygen -

docs-check:
	@cmake -E make_directory build/docs
	doxygen

size:
	$(IDF_PY) size

size-components:
	$(IDF_PY) size-components

  
reconfigure:
	$(IDF_PY) reconfigure