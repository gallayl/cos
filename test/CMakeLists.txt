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
    mocks/mock_esp_wifi.c
    mocks/mock_esp_event.c
    mocks/mock_esp_netif.c
    mocks/mock_freertos.c
    mocks/mock_i2c.c
    mocks/mock_sntp.c
    mocks/mock_system.c
    mocks/mock_httpd.c
    mocks/mock_mbedtls.c
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
    ${COMPONENT_DIR}/components/wifi/include
    ${COMPONENT_DIR}/components/i2c_bus/include
    ${COMPONENT_DIR}/components/time_sync/include
    ${COMPONENT_DIR}/components/system/include
    ${COMPONENT_DIR}/components/http_server/include
    ${COMPONENT_DIR}/components/ota/include
    ${COMPONENT_DIR}/components/websocket/include
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

# --- Library: wifi_logic ---
add_library(wifi_logic STATIC
    ${COMPONENT_DIR}/components/wifi/src/wifi.c
    ${COMPONENT_DIR}/components/wifi/src/wifi_cmd.c
)
target_include_directories(wifi_logic PUBLIC
    ${COMPONENT_DIR}/components/wifi/include
    ${COMPONENT_DIR}/components/wifi/src
    mocks
)
target_link_libraries(wifi_logic PRIVATE mock_esp)

# --- Test: wifi ---
add_executable(test_wifi test_wifi.c)
target_include_directories(test_wifi PRIVATE
    mocks
    ${COMPONENT_DIR}/components/wifi/include
)
target_link_libraries(test_wifi PRIVATE unity wifi_logic mock_esp)
add_test(NAME test_wifi COMMAND test_wifi)

# --- Library: i2c_bus_logic ---
add_library(i2c_bus_logic STATIC
    ${COMPONENT_DIR}/components/i2c_bus/src/i2c_bus.c
    ${COMPONENT_DIR}/components/i2c_bus/src/i2c_bus_cmd.c
)
target_include_directories(i2c_bus_logic PUBLIC
    ${COMPONENT_DIR}/components/i2c_bus/include
    ${COMPONENT_DIR}/components/i2c_bus/src
    mocks
)
target_link_libraries(i2c_bus_logic PRIVATE mock_esp)

# --- Test: i2c_bus ---
add_executable(test_i2c_bus test_i2c_bus.c)
target_include_directories(test_i2c_bus PRIVATE
    mocks
    ${COMPONENT_DIR}/components/i2c_bus/include
)
target_link_libraries(test_i2c_bus PRIVATE unity i2c_bus_logic mock_esp)
add_test(NAME test_i2c_bus COMMAND test_i2c_bus)

# --- Library: time_sync_logic ---
add_library(time_sync_logic STATIC
    ${COMPONENT_DIR}/components/time_sync/src/time_sync.c
    ${COMPONENT_DIR}/components/time_sync/src/time_sync_cmd.c
)
target_include_directories(time_sync_logic PUBLIC
    ${COMPONENT_DIR}/components/time_sync/include
    mocks
)
target_link_libraries(time_sync_logic PRIVATE mock_esp)

# --- Test: time_sync ---
add_executable(test_time_sync test_time_sync.c)
target_include_directories(test_time_sync PRIVATE
    mocks
    ${COMPONENT_DIR}/components/time_sync/include
)
target_link_libraries(test_time_sync PRIVATE unity time_sync_logic mock_esp)
add_test(NAME test_time_sync COMMAND test_time_sync)

# --- Library: system_logic ---
add_library(system_logic STATIC
    ${COMPONENT_DIR}/components/system/src/system.c
    ${COMPONENT_DIR}/components/system/src/system_cmd.c
)
target_include_directories(system_logic PUBLIC
    ${COMPONENT_DIR}/components/system/include
    ${COMPONENT_DIR}/components/filesystem/include
    mocks
)
target_link_libraries(system_logic PRIVATE mock_esp)

# --- Test: system ---
add_executable(test_system test_system.c)
target_include_directories(test_system PRIVATE
    mocks
    ${COMPONENT_DIR}/components/system/include
)
target_link_libraries(test_system PRIVATE unity system_logic mock_esp)
add_test(NAME test_system COMMAND test_system)

# --- Library: http_mime ---
add_library(http_mime STATIC
    ${COMPONENT_DIR}/components/http_server/src/http_mime.c
)
target_include_directories(http_mime PUBLIC
    ${COMPONENT_DIR}/components/http_server/include
    ${COMPONENT_DIR}/components/http_server/src
    mocks
)

# --- Library: http_path (sanitize path only) ---
add_library(http_path STATIC
    ${COMPONENT_DIR}/components/http_server/src/http_path.c
)
target_include_directories(http_path PUBLIC
    ${COMPONENT_DIR}/components/http_server/include
    mocks
)

# --- Library: http_auth_logic ---
add_library(http_auth_logic STATIC
    ${COMPONENT_DIR}/components/http_server/src/http_auth.c
)
target_include_directories(http_auth_logic PUBLIC
    ${COMPONENT_DIR}/components/http_server/include
    ${COMPONENT_DIR}/components/http_server/src
    mocks
)
target_link_libraries(http_auth_logic PRIVATE mock_esp)

# --- Test: http_mime ---
add_executable(test_http_mime test_http_mime.c)
target_include_directories(test_http_mime PRIVATE
    mocks
    ${COMPONENT_DIR}/components/http_server/include
)
target_link_libraries(test_http_mime PRIVATE unity http_mime mock_esp)
add_test(NAME test_http_mime COMMAND test_http_mime)

# --- Test: http_upload ---
add_executable(test_http_upload test_http_upload.c)
target_include_directories(test_http_upload PRIVATE
    mocks
    ${COMPONENT_DIR}/components/http_server/include
)
target_link_libraries(test_http_upload PRIVATE unity http_path mock_esp)
add_test(NAME test_http_upload COMMAND test_http_upload)

# --- Test: http_auth ---
add_executable(test_http_auth test_http_auth.c)
target_include_directories(test_http_auth PRIVATE
    mocks
    ${COMPONENT_DIR}/components/http_server/include
)
target_link_libraries(test_http_auth PRIVATE unity http_auth_logic mock_esp)
add_test(NAME test_http_auth COMMAND test_http_auth)
