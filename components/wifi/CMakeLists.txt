idf_component_register(
    SRCS "src/wifi.c"
         "src/wifi_cmd.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES esp_wifi esp_netif esp_event nvs_flash console freertos
)
