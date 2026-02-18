idf_component_register(
    SRCS "src/http_server.c"
         "src/http_server_cmd.c"
         "src/http_auth.c"
         "src/http_static.c"
         "src/http_api.c"
         "src/http_upload.c"
         "src/http_path.c"
         "src/http_multipart.c"
         "src/http_mime.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES esp_http_server console nvs_flash mbedtls
    PRIV_REQUIRES filesystem
)
