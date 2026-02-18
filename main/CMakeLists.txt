idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES display calibration rgb_led light_sensor brightness i2c_bus filesystem wifi time_sync system shell nvs_flash console
)
