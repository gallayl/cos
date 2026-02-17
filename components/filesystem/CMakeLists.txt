idf_component_register(
    SRCS "src/flash.c"
         "src/sdcard.c"
         "src/vfs.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES fatfs sdmmc driver
)
