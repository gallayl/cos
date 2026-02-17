idf_component_register(
    SRCS "src/calibration.c"
         "src/calibration_storage.c"
         "src/calibration_cmd.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES display nvs_flash console
)
