idf_component_register(
    SRCS "controller_bt_classic.c"
    INCLUDE_DIRS "."
    PRIV_INCLUDE_DIRS "../../main"
    REQUIRES bt nvs_flash hex_rpc_transport hex_shared_types
)
