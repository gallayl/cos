cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/../../components/filesystem
    ${CMAKE_CURRENT_LIST_DIR}/../../components/shell
    ${CMAKE_CURRENT_LIST_DIR}/../../components/system
    ${CMAKE_CURRENT_LIST_DIR}/../../components/display
    ${CMAKE_CURRENT_LIST_DIR}/../../components/ui
    ${CMAKE_CURRENT_LIST_DIR}/../../components/text_console
    ${CMAKE_CURRENT_LIST_DIR}/../../components/berry_lang
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_stability)
