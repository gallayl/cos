idf_component_register(
    SRCS "src/rgb_led.c"
         "src/rgb_led_cmd.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES nvs_flash console driver
)
