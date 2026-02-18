idf_component_register(
    SRCS "test_shell_fs.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash filesystem shell
)
