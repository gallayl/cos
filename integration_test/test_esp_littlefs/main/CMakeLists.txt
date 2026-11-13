set(TEST_SRCS
    "test_main.c"
    "test_littlefs.c"
    "test_littlefs_common.c"
    "test_dir.c"
    "test_benchmark.c"
    "test_littlefs_static_partition.c"
    "test_littlefs_raw_api.c"
)

set(TEST_REQUIRES
    esp_littlefs
    unity
    vfs
    spi_flash
    spiffs
    fatfs
    wear_levelling
    esp_timer
    esp_partition
)

if(NOT IDF_TARGET STREQUAL "esp8266" AND "${IDF_VERSION_MAJOR}" VERSION_GREATER_EQUAL "6")
    list(APPEND TEST_SRCS "test_littlefs_blockdev.c")
    list(APPEND TEST_REQUIRES esp_blockdev)
endif()

idf_component_register(
    SRCS ${TEST_SRCS}
    INCLUDE_DIRS "."
    PRIV_REQUIRES ${TEST_REQUIRES}
    WHOLE_ARCHIVE
)

target_add_binary_data(${COMPONENT_LIB} "testfs.bin" BINARY)
