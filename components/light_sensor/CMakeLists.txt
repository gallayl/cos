idf_component_register(
    SRCS "src/light_sensor.c"
         "src/light_sensor_cmd.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES esp_adc console
)
