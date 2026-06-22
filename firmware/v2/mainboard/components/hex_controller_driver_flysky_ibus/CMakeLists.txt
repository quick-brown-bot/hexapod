idf_component_register(
    SRCS "controller_flysky_ibus.c"
    INCLUDE_DIRS "."
    REQUIRES driver hex_controller_core hex_shared_types
)
