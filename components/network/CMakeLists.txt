idf_component_register(
    SRCS "src/network.c" "src/network_url.c" "src/network_http.c" "src/network_status.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer freertos lwip wifi esp_http_client esp-tls mbedtls esp_netif esp_wifi
)
