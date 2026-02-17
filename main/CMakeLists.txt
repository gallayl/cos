idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES display calibration nvs_flash console
)
