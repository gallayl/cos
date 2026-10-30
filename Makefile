.PHONY: build flash flash-game monitor clean fullclean erase-flash format format-check lint test integration-test integration-test-build integration-test-run menuconfig docs docs-check

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

build:
	$(IDF_PY) build

flash:
	$(IDF_PY) flash

flash-game: build
	cd "$(CURDIR)/build" && $(IDF_PYTHON) -m esptool --chip esp32 $(if $(PORT),-p $(PORT),) -b 460800 --before default-reset --after no-reset write-flash @littlefs-flash_args

monitor:
	$(IDF_PY) monitor

clean:
	$(IDF_PY) clean

fullclean:
	$(IDF_PY) fullclean

erase-flash:
	$(IDF_PY) erase-flash

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

integration-test-build:
	$(IDF_PY) -C integration_test/test_nvs_calibration build
	$(IDF_PY) -C integration_test/test_shell_fs build
	$(IDF_PY) -C integration_test/test_init_sequence build
	$(IDF_PY) -C integration_test/test_vfs_ops build
	$(IDF_PY) -C integration_test/test_system build
	$(IDF_PY) -C integration_test/test_stability build

integration-test-run:
	@status=0; \
	for test_dir in \
		integration_test/test_nvs_calibration \
		integration_test/test_shell_fs \
		integration_test/test_init_sequence \
		integration_test/test_vfs_ops \
		integration_test/test_system \
		integration_test/test_stability; do \
		$(IDF_PYTHON) -m pytest "$$test_dir" --target esp32 --embedded-services idf,qemu -v || status=1; \
	done; \
	exit $$status

integration-test: integration-test-build integration-test-run

docs:
	@cmake -E make_directory build/docs
	( cat Doxyfile ; echo "WARN_AS_ERROR = NO" ) | doxygen -

docs-check:
	@cmake -E make_directory build/docs
	doxygen
