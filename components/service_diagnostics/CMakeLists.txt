idf_component_register(
    SRCS "src/service_diagnostics.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_common esp_system system berry_lang network wifi bluetooth text_console http_server websocket time_sync
)
