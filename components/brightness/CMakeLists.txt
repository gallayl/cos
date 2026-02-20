idf_component_register(
    SRCS "src/brightness.c"
         "src/brightness_cmd.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES display light_sensor calibration text_console nvs_flash console esp_timer
)
