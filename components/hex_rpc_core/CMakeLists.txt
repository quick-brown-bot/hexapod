idf_component_register(
    SRCS "rpc_commands.c"
    INCLUDE_DIRS "."
    PRIV_INCLUDE_DIRS "../../main"
    REQUIRES
    hex_rpc_transport
    hex_config_manager
    hex_shared_types
)
