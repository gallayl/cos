idf_component_register(
    SRCS "test_system.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash filesystem shell system
)
