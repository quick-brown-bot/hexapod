#include "robot_config.h"
#include "config_manager_runtime_api.h"
#include "config_ns_joint_api.h"
#include "config_ns_leg_geometry_api.h"
#include <string.h>
#include "esp_log.h"

// Single static instance for now. In the future, load/save to NVS.
static robot_config_t g_cfg;
static float g_base_x[NUM_LEGS];
static float g_base_y[NUM_LEGS];
static float g_base_z[NUM_LEGS];
static float g_base_yaw[NUM_LEGS];
static float g_stance_out[NUM_LEGS]; // leg-local outward (+)
static float g_stance_fwd[NUM_LEGS]; // leg-local forward (+)
static const char* TAG = "robot_config";

esp_err_t robot_config_init_default(void) {
    memset(&g_cfg, 0, sizeof(g_cfg));
    // Default: no GPIOs assigned yet; distribute legs across two MCPWM groups (0 for legs 0..2, 1 for legs 3..5)
    for (int i = 0; i < NUM_LEGS; ++i) {
        g_cfg.mcpwm_group_id[i] = (i < 3) ? 0 : 1;
        for (int j = 0; j < 3; ++j) g_cfg.servo_gpio[i][j] = -1;
        for (int j = 0; j < 3; ++j) g_cfg.servo_driver_sel[i][j] = 0; // default MCPWM
    }
    // Rebalance within first three legs: move left-rear leg to group 1 so group 0 only has two legs (6 channels)
    g_cfg.mcpwm_group_id[LEG_LEFT_REAR] = 1;

    // GPIO assignments by leg index enum

    g_cfg.servo_gpio[LEG_LEFT_FRONT][LEG_SERVO_COXA]  = 27;
    g_cfg.servo_gpio[LEG_LEFT_FRONT][LEG_SERVO_FEMUR] = 13;
    g_cfg.servo_gpio[LEG_LEFT_FRONT][LEG_SERVO_TIBIA] = 12;

    g_cfg.servo_gpio[LEG_LEFT_MIDDLE][LEG_SERVO_COXA]  = 14;
    g_cfg.servo_gpio[LEG_LEFT_MIDDLE][LEG_SERVO_FEMUR] = 26;
    g_cfg.servo_gpio[LEG_LEFT_MIDDLE][LEG_SERVO_TIBIA] = 25;

    g_cfg.servo_gpio[LEG_LEFT_REAR][LEG_SERVO_COXA]  = 23;
    g_cfg.servo_gpio[LEG_LEFT_REAR][LEG_SERVO_FEMUR] = 32;
    g_cfg.servo_gpio[LEG_LEFT_REAR][LEG_SERVO_TIBIA] = 33;

    g_cfg.servo_gpio[LEG_RIGHT_FRONT][LEG_SERVO_COXA]  = 5;
    g_cfg.servo_gpio[LEG_RIGHT_FRONT][LEG_SERVO_FEMUR] = 17;
    g_cfg.servo_gpio[LEG_RIGHT_FRONT][LEG_SERVO_TIBIA] = 16;

    g_cfg.servo_gpio[LEG_RIGHT_MIDDLE][LEG_SERVO_COXA]  = 4;
    g_cfg.servo_gpio[LEG_RIGHT_MIDDLE][LEG_SERVO_FEMUR] = 2;
    g_cfg.servo_gpio[LEG_RIGHT_MIDDLE][LEG_SERVO_TIBIA] = 15;

    g_cfg.servo_gpio[LEG_RIGHT_REAR][LEG_SERVO_COXA]  = 21;
    g_cfg.servo_gpio[LEG_RIGHT_REAR][LEG_SERVO_FEMUR] = 19;
    g_cfg.servo_gpio[LEG_RIGHT_REAR][LEG_SERVO_TIBIA] = 18;

    // Driver selection: choose LEDC for the three joints of RIGHT_MIDDLE (leg 4) and LEFT_REAR (leg 2) as example.
    // Adjust to match pins that are LEDC-friendly if some MCPWM pins (like 32/33/34/35) cause LEDC invalid gpio errors.
    // For now we move LEDC usage away from GPIO 32/35 which are input-only or restricted on classic ESP32.
    for (int j = 0; j < 3; ++j) {
        g_cfg.servo_driver_sel[LEG_RIGHT_MIDDLE][j] = 1; // LEDC
        g_cfg.servo_driver_sel[LEG_RIGHT_REAR][j] = 1;   // LEDC (example second leg)
    }

    // These offsets are kinematic-model constants used by leg_configure.
    // Lengths/mount/stance must come from the leg geometry namespace.
    const float coxa_offset_rad = 0.0f;
    const float femur_offset_rad = 0.5396943301595464f;
    const float tibia_offset_rad = 1.0160719600939494f;

    config_manager_state_t cfg_state = {0};

    if (config_manager_get_state(&cfg_state) != ESP_OK ||
        !cfg_state.initialized ||
        !cfg_state.namespace_loaded[CONFIG_NS_LEG_GEOMETRY] ||
        !cfg_state.namespace_loaded[CONFIG_NS_JOINT_CALIB]) {
        ESP_LOGE(TAG, "Required configuration namespaces are not loaded");
        return ESP_ERR_INVALID_STATE;
    }

    const leg_geometry_config_t* stored_geom = config_get_leg_geometry();
    if (!stored_geom) {
        ESP_LOGE(TAG, "Leg geometry namespace returned NULL config");
        return ESP_ERR_INVALID_STATE;
    }

    for (int i = 0; i < NUM_LEGS; ++i) {
        leg_geometry_t leg_geom = {
            .len_coxa = stored_geom->len_coxa[i],
            .len_femur = stored_geom->len_femur[i],
            .len_tibia = stored_geom->len_tibia[i],
            .coxa_offset_rad = coxa_offset_rad,
            .femur_offset_rad = femur_offset_rad,
            .tibia_offset_rad = tibia_offset_rad,
        };
        (void)leg_configure(&leg_geom, &g_cfg.legs[i]);
    }

    // --- Joint calibration ---
    // Joint calibration is now handled directly by config_manager.
    // No caching needed - robot_config_get_joint_calib() calls config_manager on-demand.

    for (int i = 0; i < NUM_LEGS; ++i) {
        g_base_x[i] = stored_geom->mount_x[i];
        g_base_y[i] = stored_geom->mount_y[i];
        g_base_z[i] = stored_geom->mount_z[i];
        g_base_yaw[i] = stored_geom->mount_yaw[i];
        g_stance_out[i] = stored_geom->stance_out[i];
        g_stance_fwd[i] = stored_geom->stance_fwd[i];
    }

    // --- Future hardware settings (not applied here; live in robot_control) ---
    // - Servo pins and MCPWM mapping
    // - Per-joint limits/offsets/inversions
    // - Mount poses (position + yaw)

    // --- Debug defaults ---
    g_cfg.debug_leg_enable = 1;         // enable by default for bring-up
    g_cfg.debug_leg_index = 0;          // monitor leg 0 by default
    g_cfg.debug_leg_delta_thresh = 0.0174533f; // ~1 degree
    g_cfg.debug_leg_min_interval_ms = 100;     // 100 ms between logs min

    return ESP_OK;
}

leg_handle_t robot_config_get_leg(int leg_index) {
    if (leg_index < 0 || leg_index >= NUM_LEGS) return NULL;
    return g_cfg.legs[leg_index];
}

void robot_config_get_base_pose(int leg_index, float *x, float *y, float *z, float *yaw) {
    if (x)   *x = (leg_index >= 0 && leg_index < NUM_LEGS) ? g_base_x[leg_index] : 0.0f;
    if (y)   *y = (leg_index >= 0 && leg_index < NUM_LEGS) ? g_base_y[leg_index] : 0.0f;
    if (z)   *z = (leg_index >= 0 && leg_index < NUM_LEGS) ? g_base_z[leg_index] : 0.0f;
    if (yaw) *yaw = (leg_index >= 0 && leg_index < NUM_LEGS) ? g_base_yaw[leg_index] : 0.0f;
}

const joint_calib_t* robot_config_get_joint_calib(int leg_index, leg_servo_t joint) {
    if (leg_index < 0 || leg_index >= NUM_LEGS) return NULL;
    int j = (int)joint;
    if (j < 0 || j >= 3) return NULL;
    
    // Static storage for calibration data
    static joint_calib_t temp_calib;
    
    // Get data directly from configuration manager
    if (config_get_joint_calib_data(leg_index, j, &temp_calib) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read joint calibration for leg %d joint %d", leg_index, j);
        return NULL;
    }

    return &temp_calib;
}

int robot_config_get_servo_gpio(int leg_index, leg_servo_t joint) {
    if (leg_index < 0 || leg_index >= NUM_LEGS) return -1;
    int j = (int)joint;
    if (j < 0 || j >= 3) return -1;
    return g_cfg.servo_gpio[leg_index][j];
}

int robot_config_get_mcpwm_group(int leg_index) {
    if (leg_index < 0 || leg_index >= NUM_LEGS) return 0;
    return g_cfg.mcpwm_group_id[leg_index];
}

int robot_config_get_servo_driver(int leg_index, leg_servo_t joint) {
    if (leg_index < 0 || leg_index >= NUM_LEGS) return 0;
    int j = (int)joint; if (j < 0 || j >= 3) return 0;
    return g_cfg.servo_driver_sel[leg_index][j];
}

int robot_config_debug_enabled(void) {
    return g_cfg.debug_leg_enable;
}
int robot_config_debug_leg_index(void) {
    return g_cfg.debug_leg_index;
}
float robot_config_debug_delta_thresh(void) {
    return g_cfg.debug_leg_delta_thresh;
}

float robot_config_get_stance_out_m(int leg_index) {
    if (leg_index < 0 || leg_index >= NUM_LEGS) return 0.0f;
    return g_stance_out[leg_index];
}
float robot_config_get_stance_fwd_m(int leg_index) {
    if (leg_index < 0 || leg_index >= NUM_LEGS) return 0.0f;
    return g_stance_fwd[leg_index];
}


