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
    mocks/mock_filesystem.c
    mocks/mock_console.c
    mocks/mock_ledc.c
    mocks/mock_light_sensor.c
    mocks/mock_esp_timer.c
)
target_include_directories(mock_esp PUBLIC
    mocks
    ${COMPONENT_DIR}/components/display/include
    ${COMPONENT_DIR}/components/calibration/include
    ${COMPONENT_DIR}/components/calibration/src
    ${COMPONENT_DIR}/components/filesystem/include
    ${COMPONENT_DIR}/components/shell/include
    ${COMPONENT_DIR}/components/rgb_led/include
    ${COMPONENT_DIR}/components/light_sensor/include
    ${COMPONENT_DIR}/components/brightness/include
)

add_library(vfs_path STATIC
    ${COMPONENT_DIR}/components/filesystem/src/vfs_path.c
)
target_include_directories(vfs_path PUBLIC
    ${COMPONENT_DIR}/components/filesystem/include
    ${COMPONENT_DIR}/components/filesystem/src
    mocks
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

add_library(shell_logic STATIC
    ${COMPONENT_DIR}/components/shell/src/shell.c
    ${COMPONENT_DIR}/components/shell/src/shell_fs_cmds.c
    ${COMPONENT_DIR}/components/shell/src/shell_sd_cmds.c
    ${COMPONENT_DIR}/components/shell/src/shell_info_cmds.c
)
target_include_directories(shell_logic PUBLIC
    ${COMPONENT_DIR}/components/shell/include
    ${COMPONENT_DIR}/components/shell/src
    ${COMPONENT_DIR}/components/filesystem/include
    mocks
)
target_link_libraries(shell_logic PRIVATE mock_esp)

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

# --- Test: vfs_path ---
add_executable(test_vfs_path test_vfs_path.c)
target_include_directories(test_vfs_path PRIVATE
    mocks
    ${COMPONENT_DIR}/components/filesystem/include
)
target_link_libraries(test_vfs_path PRIVATE unity vfs_path mock_esp)
add_test(NAME test_vfs_path COMMAND test_vfs_path)

# --- Test: shell ---
add_executable(test_shell test_shell.c)
target_include_directories(test_shell PRIVATE
    mocks
    ${COMPONENT_DIR}/components/filesystem/include
    ${COMPONENT_DIR}/components/shell/include
)
target_link_libraries(test_shell PRIVATE unity shell_logic mock_esp)
add_test(NAME test_shell COMMAND test_shell)

# --- Library: rgb_led_logic ---
add_library(rgb_led_logic STATIC
    ${COMPONENT_DIR}/components/rgb_led/src/rgb_led.c
    ${COMPONENT_DIR}/components/rgb_led/src/rgb_led_cmd.c
)
target_include_directories(rgb_led_logic PUBLIC
    ${COMPONENT_DIR}/components/rgb_led/include
    ${COMPONENT_DIR}/components/rgb_led/src
    mocks
)
target_link_libraries(rgb_led_logic PRIVATE mock_esp)

# --- Test: rgb_led ---
add_executable(test_rgb_led test_rgb_led.c)
target_include_directories(test_rgb_led PRIVATE
    mocks
    ${COMPONENT_DIR}/components/rgb_led/include
)
target_link_libraries(test_rgb_led PRIVATE unity rgb_led_logic mock_esp)
add_test(NAME test_rgb_led COMMAND test_rgb_led)

# --- Library: brightness_logic ---
add_library(brightness_logic STATIC
    ${COMPONENT_DIR}/components/brightness/src/brightness.c
    ${COMPONENT_DIR}/components/brightness/src/brightness_cmd.c
)
target_include_directories(brightness_logic PUBLIC
    ${COMPONENT_DIR}/components/brightness/include
    ${COMPONENT_DIR}/components/brightness/src
    ${COMPONENT_DIR}/components/display/include
    ${COMPONENT_DIR}/components/light_sensor/include
    ${COMPONENT_DIR}/components/calibration/include
    mocks
)
target_link_libraries(brightness_logic PRIVATE mock_esp)

# --- Test: brightness ---
add_executable(test_brightness test_brightness.c)
target_include_directories(test_brightness PRIVATE
    mocks
    ${COMPONENT_DIR}/components/brightness/include
    ${COMPONENT_DIR}/components/display/include
    ${COMPONENT_DIR}/components/light_sensor/include
    ${COMPONENT_DIR}/components/calibration/include
)
target_link_libraries(test_brightness PRIVATE unity brightness_logic calibration_logic calibration_storage mock_esp)
add_test(NAME test_brightness COMMAND test_brightness)
