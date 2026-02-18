idf_component_register(
    SRCS "src/websocket.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_server console
    PRIV_REQUIRES http_server
)
