idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES display calibration rgb_led light_sensor brightness filesystem shell nvs_flash console
)
