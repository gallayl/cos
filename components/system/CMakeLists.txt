idf_component_register(
    SRCS "src/system.c"
         "src/system_cmd.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer console
    PRIV_REQUIRES filesystem
)
