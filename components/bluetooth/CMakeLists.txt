idf_component_register(
    SRCS "src/bluetooth.c"
         "src/bluetooth_cmd.c"
         "src/bluetooth_hid.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES bt console esp_hid nvs_flash freertos
)
