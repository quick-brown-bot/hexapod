idf_component_register(
    SRCS
    "config_storage_nvs.c"
    "config_registry.c"
    "config_namespace_registry.c"
    "config_migration.c"
    "config_manager.c"
    "config_domain_system_access.c"
    "config_domain_persistence_nvs.c"
    "config_domain_joint_access.c"
    "config_domain_defaults.c"
    INCLUDE_DIRS "."
    PRIV_INCLUDE_DIRS "../../main"
    REQUIRES
    nvs_flash
    esp_partition
)
