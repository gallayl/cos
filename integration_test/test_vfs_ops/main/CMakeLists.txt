idf_component_register(
    SRCS "test_vfs_ops.c"
    INCLUDE_DIRS "."
    REQUIRES unity nvs_flash filesystem
)
