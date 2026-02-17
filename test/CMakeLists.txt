cmake_minimum_required(VERSION 3.16)
project(cos_tests C)

enable_testing()

include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.6.0
)
FetchContent_MakeAvailable(unity)

set(COMPONENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(mock_esp STATIC
    mocks/mock_nvs.c
    mocks/mock_display.c
)
target_include_directories(mock_esp PUBLIC
    mocks
    ${COMPONENT_DIR}/components/display/include
    ${COMPONENT_DIR}/components/calibration/include
    ${COMPONENT_DIR}/components/calibration/src
)

add_library(calibration_storage STATIC
    ${COMPONENT_DIR}/components/calibration/src/calibration_storage.c
)
target_include_directories(calibration_storage PUBLIC
    ${COMPONENT_DIR}/components/calibration/src
    ${COMPONENT_DIR}/components/display/include
    mocks
)
target_link_libraries(calibration_storage PRIVATE mock_esp)

add_library(calibration_logic STATIC
    ${COMPONENT_DIR}/components/calibration/src/calibration.c
    ${COMPONENT_DIR}/components/calibration/src/calibration_cmd.c
)
target_include_directories(calibration_logic PUBLIC
    ${COMPONENT_DIR}/components/calibration/include
    ${COMPONENT_DIR}/components/calibration/src
    ${COMPONENT_DIR}/components/display/include
    mocks
)
target_link_libraries(calibration_logic PRIVATE mock_esp calibration_storage)

# --- Test: calibration_storage ---
add_executable(test_calibration_storage test_calibration_storage.c)
target_include_directories(test_calibration_storage PRIVATE
    mocks
    ${COMPONENT_DIR}/components/display/include
    ${COMPONENT_DIR}/components/calibration/src
)
target_link_libraries(test_calibration_storage PRIVATE unity calibration_storage mock_esp)
add_test(NAME test_calibration_storage COMMAND test_calibration_storage)

# --- Test: calibration (orchestration) ---
add_executable(test_calibration test_calibration.c)
target_include_directories(test_calibration PRIVATE
    mocks
    ${COMPONENT_DIR}/components/display/include
    ${COMPONENT_DIR}/components/calibration/include
    ${COMPONENT_DIR}/components/calibration/src
)
target_link_libraries(test_calibration PRIVATE unity calibration_logic calibration_storage mock_esp)
add_test(NAME test_calibration COMMAND test_calibration)
