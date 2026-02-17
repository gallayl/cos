idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES display calibration filesystem shell nvs_flash console
)
