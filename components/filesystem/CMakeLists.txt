idf_component_register(
    SRCS "src/flash.c"
         "src/sdcard.c"
         "src/vfs.c"
         "src/vfs_path.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES fatfs sdmmc driver
)
