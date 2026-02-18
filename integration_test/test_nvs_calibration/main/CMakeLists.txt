idf_component_register(
    SRCS "test_nvs_cal.c"
    INCLUDE_DIRS "."
    REQUIRES unity nvs_flash calibration
)
