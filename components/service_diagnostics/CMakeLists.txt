set(reqs esp_common esp_system system berry_lang network wifi text_console)
if(CONFIG_BT_ENABLED)
    list(APPEND reqs bluetooth)
endif()

idf_component_register(
    SRCS "src/service_diagnostics.c"
    INCLUDE_DIRS "include"
    REQUIRES ${reqs}
)
