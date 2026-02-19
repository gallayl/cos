idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES display calibration rgb_led light_sensor brightness text_console i2c_bus filesystem wifi bluetooth time_sync http_server websocket system shell nvs_flash console
)
