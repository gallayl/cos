idf_component_register(
    SRCS "src/time_sync.c"
         "src/time_sync_cmd.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_netif console
)
