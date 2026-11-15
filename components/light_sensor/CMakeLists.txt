idf_component_register(
    SRCS "src/light_sensor.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES esp_adc
)
