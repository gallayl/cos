idf_component_register(
    SRCS "test_stability.c"
    INCLUDE_DIRS "."
    REQUIRES unity nvs_flash filesystem shell system esp_timer
)
