idf_component_register(
    SRCS "src/shell.c"
         "src/shell_fs_cmds.c"
         "src/shell_sd_cmds.c"
         "src/shell_info_cmds.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES filesystem console
)
