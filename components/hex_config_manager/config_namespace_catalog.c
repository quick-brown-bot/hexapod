#include "config_namespace_catalog.h"

#include "config_namespace_registry.h"
#include "config_ns_controller.h"
#include "config_ns_joint_calib.h"
#include "config_ns_leg_geometry.h"
#include "config_ns_motion_limits.h"
#include "config_ns_system.h"
#include "config_ns_wifi.h"

#include <stddef.h>

static system_config_t g_system_config = {0};
static joint_calib_config_t g_joint_calib_config = {0};
static leg_geometry_config_t g_leg_geometry_config = {0};
static motion_limits_config_t g_motion_limits_config = {0};
static controller_config_namespace_t g_controller_config = {0};
static wifi_config_namespace_t g_wifi_config = {0};

static config_system_namespace_context_t g_system_namespace_ctx = {0};
static config_joint_calib_namespace_context_t g_joint_namespace_ctx = {0};
static config_leg_geometry_namespace_context_t g_leg_geometry_namespace_ctx = {0};
static config_motion_limits_namespace_context_t g_motion_limits_namespace_ctx = {0};
static config_controller_namespace_context_t g_controller_namespace_ctx = {0};
static config_wifi_namespace_context_t g_wifi_namespace_ctx = {0};

typedef struct {
    const config_namespace_descriptor_t* descriptor;
    void* context;
} namespace_registration_entry_t;

static namespace_registration_entry_t g_namespace_registration_entries[] = {
    { &g_system_namespace_descriptor, &g_system_namespace_ctx },
    { &g_joint_namespace_descriptor, &g_joint_namespace_ctx },
    { &g_leg_geometry_namespace_descriptor, &g_leg_geometry_namespace_ctx },
    { &g_motion_limits_namespace_descriptor, &g_motion_limits_namespace_ctx },
    { &g_controller_namespace_descriptor, &g_controller_namespace_ctx },
    { &g_wifi_namespace_descriptor, &g_wifi_namespace_ctx },
};

static void bind_namespace_contexts(
    config_manager_state_t* state,
    uint16_t schema_version,
    controller_driver_type_e fallback_controller_type
) {
    g_system_namespace_ctx.nvs_handle = &state->nvs_handles[CONFIG_NS_SYSTEM];
    g_system_namespace_ctx.namespace_dirty = &state->namespace_dirty[CONFIG_NS_SYSTEM];
    g_system_namespace_ctx.namespace_loaded = &state->namespace_loaded[CONFIG_NS_SYSTEM];
    g_system_namespace_ctx.config = &g_system_config;
    g_system_namespace_ctx.schema_version = schema_version;
    g_system_namespace_ctx.fallback_controller_type = fallback_controller_type;

    g_joint_namespace_ctx.nvs_handle = &state->nvs_handles[CONFIG_NS_JOINT_CALIB];
    g_joint_namespace_ctx.namespace_dirty = &state->namespace_dirty[CONFIG_NS_JOINT_CALIB];
    g_joint_namespace_ctx.namespace_loaded = &state->namespace_loaded[CONFIG_NS_JOINT_CALIB];
    g_joint_namespace_ctx.config = &g_joint_calib_config;

    g_leg_geometry_namespace_ctx.nvs_handle = &state->nvs_handles[CONFIG_NS_LEG_GEOMETRY];
    g_leg_geometry_namespace_ctx.namespace_dirty = &state->namespace_dirty[CONFIG_NS_LEG_GEOMETRY];
    g_leg_geometry_namespace_ctx.namespace_loaded = &state->namespace_loaded[CONFIG_NS_LEG_GEOMETRY];
    g_leg_geometry_namespace_ctx.config = &g_leg_geometry_config;

    g_motion_limits_namespace_ctx.nvs_handle = &state->nvs_handles[CONFIG_NS_MOTION_LIMITS];
    g_motion_limits_namespace_ctx.namespace_dirty = &state->namespace_dirty[CONFIG_NS_MOTION_LIMITS];
    g_motion_limits_namespace_ctx.namespace_loaded = &state->namespace_loaded[CONFIG_NS_MOTION_LIMITS];
    g_motion_limits_namespace_ctx.config = &g_motion_limits_config;

    g_controller_namespace_ctx.nvs_handle = &state->nvs_handles[CONFIG_NS_CONTROLLER];
    g_controller_namespace_ctx.namespace_dirty = &state->namespace_dirty[CONFIG_NS_CONTROLLER];
    g_controller_namespace_ctx.namespace_loaded = &state->namespace_loaded[CONFIG_NS_CONTROLLER];
    g_controller_namespace_ctx.config = &g_controller_config;

    g_wifi_namespace_ctx.nvs_handle = &state->nvs_handles[CONFIG_NS_WIFI];
    g_wifi_namespace_ctx.namespace_dirty = &state->namespace_dirty[CONFIG_NS_WIFI];
    g_wifi_namespace_ctx.namespace_loaded = &state->namespace_loaded[CONFIG_NS_WIFI];
    g_wifi_namespace_ctx.config = &g_wifi_config;
}

esp_err_t config_namespace_catalog_register_all(
    config_manager_state_t* state,
    uint16_t schema_version,
    controller_driver_type_e fallback_controller_type
) {
    if (!state) {
        return ESP_ERR_INVALID_ARG;
    }

    bind_namespace_contexts(state, schema_version, fallback_controller_type);

    config_namespace_registry_reset();

    size_t descriptor_count = sizeof(g_namespace_registration_entries) / sizeof(g_namespace_registration_entries[0]);
    for (size_t i = 0; i < descriptor_count; i++) {
        const config_namespace_descriptor_t* descriptor = g_namespace_registration_entries[i].descriptor;
        void* context = g_namespace_registration_entries[i].context;
        esp_err_t err = config_namespace_registry_register(descriptor, context);
        if (err != ESP_OK) {
            return err;
        }
    }

    return ESP_OK;
}

system_config_t* config_namespace_catalog_system_config(void) {
    return &g_system_config;
}

joint_calib_config_t* config_namespace_catalog_joint_calib_config(void) {
    return &g_joint_calib_config;
}

leg_geometry_config_t* config_namespace_catalog_leg_geometry_config(void) {
    return &g_leg_geometry_config;
}

motion_limits_config_t* config_namespace_catalog_motion_limits_config(void) {
    return &g_motion_limits_config;
}

controller_config_namespace_t* config_namespace_catalog_controller_config(void) {
    return &g_controller_config;
}

wifi_config_namespace_t* config_namespace_catalog_wifi_config(void) {
    return &g_wifi_config;
}
