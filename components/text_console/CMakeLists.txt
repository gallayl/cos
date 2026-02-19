idf_component_register(
    SRCS "src/text_console.cpp" "src/text_buffer.c" "src/text_console_cmd.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    PRIV_REQUIRES display console
)
