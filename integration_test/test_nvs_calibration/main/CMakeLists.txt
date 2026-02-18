idf_component_register(
    SRCS "test_nvs_cal.c"
    INCLUDE_DIRS "."
    PRIV_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../../components/calibration/src"
    REQUIRES unity nvs_flash calibration
)
