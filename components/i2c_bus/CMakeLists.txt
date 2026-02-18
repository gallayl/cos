idf_component_register(
    SRCS "src/i2c_bus.c"
         "src/i2c_bus_cmd.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES driver console
)
