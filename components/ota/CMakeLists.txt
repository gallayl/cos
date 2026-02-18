idf_component_register(
    SRCS "src/ota.c"
    INCLUDE_DIRS "include"
    REQUIRES app_update console
    PRIV_REQUIRES http_server
)
