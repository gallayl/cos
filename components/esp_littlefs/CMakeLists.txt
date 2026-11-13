cmake_minimum_required(VERSION 3.16)

set(SOURCES
    src/esp_littlefs.c
    src/lfs_config.c
    src/littlefs_esp_part.c
    src/littlefs/lfs.c
    src/littlefs/lfs_util.c
)

if(NOT IDF_TARGET STREQUAL "esp8266" AND "${IDF_VERSION_MAJOR}" VERSION_GREATER_EQUAL "6")
    list(APPEND SOURCES src/littlefs_bdl.c)
endif()

if(NOT IDF_TARGET STREQUAL "esp8266")
    list(APPEND pub_requires sdmmc)
    if(CONFIG_LITTLEFS_SDMMC_SUPPORT)
        list(APPEND SOURCES src/littlefs_sdmmc.c)
    endif()
endif()

list(APPEND pub_requires esp_partition)
if(NOT IDF_TARGET STREQUAL "esp8266" AND "${IDF_VERSION_MAJOR}" VERSION_GREATER_EQUAL "6")
    list(APPEND pub_requires esp_blockdev)
endif()
list(APPEND priv_requires esptool_py spi_flash vfs)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS include
    PRIV_INCLUDE_DIRS src src/littlefs
    REQUIRES ${pub_requires}
    PRIV_REQUIRES ${priv_requires}
)

set_source_files_properties(
    ${SOURCES}
    PROPERTIES COMPILE_FLAGS "-DLFS_CONFIG=lfs_config.h"
)

if(CONFIG_LITTLEFS_FCNTL_GET_PATH)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC -DF_GETPATH=${CONFIG_LITTLEFS_FCNTL_F_GETPATH_VALUE})
endif()
