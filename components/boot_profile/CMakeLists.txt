idf_component_register(
    SRCS "src/boot_profile.c" "src/boot_profile_cmd.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash console
)
