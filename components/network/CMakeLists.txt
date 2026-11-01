idf_component_register(
    SRCS "src/network.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer freertos lwip wifi
)
