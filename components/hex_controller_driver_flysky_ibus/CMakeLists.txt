idf_component_register(
    SRCS "controller_flysky_ibus.c"
    INCLUDE_DIRS "."
    PRIV_INCLUDE_DIRS "../../main"
    REQUIRES driver hex_shared_types
)
