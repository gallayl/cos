idf_component_register(
    SRCS "test_init.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash filesystem shell
)
