.PHONY: build flash monitor clean fullclean format format-check lint test menuconfig

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
           $(wildcard components/filesystem/include/*.h) \
           $(wildcard components/filesystem/src/*.c components/filesystem/src/*.h) \
           $(wildcard components/shell/include/*.h) \
           $(wildcard components/shell/src/*.c components/shell/src/*.h)

build:
	$(IDF_PY) build

flash:
	$(IDF_PY) flash

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
	clang-tidy -p build $(SOURCES)

test:
	cmake -B test/build test
	cmake --build test/build
	ctest --test-dir test/build --output-on-failure
