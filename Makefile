.PHONY: build flash flash-data monitor clean fullclean erase-flash format format-check lint lint-db test integration-test integration-test-build integration-test-run menuconfig docs docs-check

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

# Optional compiler cache (DX3): set USE_CCACHE=1 (or IDF_CCACHE_ENABLE=1) to
# enable ESP-IDF ccache/sccache integration for faster rebuilds. If the ccache
# executable is installed under IDF tools but not on PATH, add it automatically.
USE_CCACHE ?=
ifneq ($(strip $(USE_CCACHE)),)
  ifneq ($(filter 0 false FALSE no NO,$(USE_CCACHE)),0 false FALSE no NO)
    IDF_CCACHE_ENABLE ?= 1
  endif
endif
ifneq ($(strip $(IDF_CCACHE_ENABLE)),)
  ifneq ($(filter 1 true TRUE yes YES,$(IDF_CCACHE_ENABLE)),)
    SEP ?= $(if $(findstring ;,$(PATH)),;,:)
    ifdef IDF_TOOLS_PATH
      CCACHE_TOOL_DIRS := $(dir $(wildcard $(IDF_TOOLS_PATH)/ccache/*/*/ccache.exe) $(wildcard $(IDF_TOOLS_PATH)/ccache/*/*/ccache) $(wildcard $(IDF_TOOLS_PATH)/ccache/*/ccache.exe) $(wildcard $(IDF_TOOLS_PATH)/ccache/*/ccache))
      ifneq ($(strip $(CCACHE_TOOL_DIRS)),)
        export PATH := $(subst $(eval ) ,$(SEP),$(CCACHE_TOOL_DIRS))$(SEP)$(PATH)
      endif
    endif
  endif
endif
ifdef IDF_CCACHE_ENABLE
  export IDF_CCACHE_ENABLE
endif

# Deterministic source discovery for format-check/lint (portable: GNU/Windows Make).
# Covers project-owned C/C++/headers under main and component include/src trees.
FORMAT_SOURCES := $(sort \
	$(wildcard main/*.c main/*.h) \
	$(wildcard components/*/include/*.h) \
	$(wildcard components/*/src/*.c) \
	$(wildcard components/*/src/*.cpp) \
	$(wildcard components/*/src/*.h) \
	$(wildcard components/esp_littlefs/src/littlefs/*.c) \
	$(wildcard components/esp_littlefs/src/littlefs/*.h))
# Vendored esp_littlefs keeps its own style; lint-db still covers it via the build graph.
SOURCES := $(filter-out components/esp_littlefs/%,$(FORMAT_SOURCES))

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
	clang-format -i $(FORMAT_SOURCES)

format-check:
	clang-format --dry-run --Werror $(FORMAT_SOURCES)

lint-db:
	@$(IDF_PYTHON) scripts/prepare_lint_db.py

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
	$(IDF_PYTHON) scripts/integration_test.py build $(INTEGRATION_TEST_BUILD_ARGS) $(if $(INTEGRATION_TEST_JOBS),-j $(INTEGRATION_TEST_JOBS),)

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