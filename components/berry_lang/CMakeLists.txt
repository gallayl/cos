if(CMAKE_BUILD_EARLY_EXPANSION)
    idf_component_register(REQUIRES console filesystem text_console ui esp_timer)
    return()
endif()

set(BERRY_REVISION "65fce31732a9bdb91cb42cfc86e2b40f16c57514")
set(COS_BERRY_SOURCE_DIR "${CMAKE_BINARY_DIR}/berry-source" CACHE PATH "Path to the pinned Berry source checkout")

if(NOT EXISTS "${COS_BERRY_SOURCE_DIR}/src/berry.h")
    execute_process(
        COMMAND git clone https://github.com/berry-lang/berry.git "${COS_BERRY_SOURCE_DIR}"
        RESULT_VARIABLE BERRY_CLONE_RESULT
        ERROR_VARIABLE BERRY_CLONE_ERROR
    )
    if(NOT BERRY_CLONE_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to fetch Berry source: ${BERRY_CLONE_ERROR}")
    endif()
endif()

execute_process(
    COMMAND git -C "${COS_BERRY_SOURCE_DIR}" checkout --detach "${BERRY_REVISION}"
    RESULT_VARIABLE BERRY_CHECKOUT_RESULT
    ERROR_VARIABLE BERRY_CHECKOUT_ERROR
)
if(NOT BERRY_CHECKOUT_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to check out Berry ${BERRY_REVISION}: ${BERRY_CHECKOUT_ERROR}")
endif()

execute_process(
    COMMAND git -C "${COS_BERRY_SOURCE_DIR}" rev-parse HEAD
    RESULT_VARIABLE BERRY_REVISION_RESULT
    OUTPUT_VARIABLE BERRY_CHECKED_OUT_REVISION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_VARIABLE BERRY_REVISION_ERROR
)
if(NOT BERRY_REVISION_RESULT EQUAL 0 OR NOT BERRY_CHECKED_OUT_REVISION STREQUAL BERRY_REVISION)
    message(FATAL_ERROR "Berry checkout does not match pinned revision ${BERRY_REVISION}: ${BERRY_REVISION_ERROR}")
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(BERRY_GENERATE_DIR "${COS_BERRY_SOURCE_DIR}/generate")
set(BERRY_MARKER "${BERRY_GENERATE_DIR}/be_const_strtab.h")
file(MAKE_DIRECTORY "${BERRY_GENERATE_DIR}")

add_custom_command(
    OUTPUT "${BERRY_MARKER}"
    COMMAND "${Python3_EXECUTABLE}" "${COS_BERRY_SOURCE_DIR}/tools/coc/coc"
        -o "${BERRY_GENERATE_DIR}"
        "${COS_BERRY_SOURCE_DIR}/src"
        "${COS_BERRY_SOURCE_DIR}/default"
        -c "${CMAKE_CURRENT_SOURCE_DIR}/src/berry_conf.h"
    DEPENDS
        "${COS_BERRY_SOURCE_DIR}/tools/coc/coc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/berry_conf.h"
    WORKING_DIRECTORY "${COS_BERRY_SOURCE_DIR}"
    COMMENT "Generating Berry constant tables"
    VERBATIM
)
add_custom_target(berry_generate DEPENDS "${BERRY_MARKER}")

file(GLOB BERRY_CORE_SOURCES "${COS_BERRY_SOURCE_DIR}/src/*.c")

idf_component_register(
    SRCS
        ${BERRY_CORE_SOURCES}
        "src/be_modtab.c"
        "src/be_port.c"
        "src/berry_ui.cpp"
        "src/berry_runtime.cpp"
    INCLUDE_DIRS
        "include"
        "src"
        "${COS_BERRY_SOURCE_DIR}/src"
        "${COS_BERRY_SOURCE_DIR}/default"
        "${COS_BERRY_SOURCE_DIR}/generate"
    REQUIRES console filesystem text_console ui esp_timer
)

add_dependencies(${COMPONENT_LIB} berry_generate)
