#include "config_domain_persistence_nvs.h"

#include <stdio.h>
#include <string.h>

#include "config_domain_defaults.h"
#include "config_manager.h"
#include "esp_log.h"
#include "esp_mac.h"

static const char *TAG = "cfg_domain_nvs";

#define SYS_KEY_EMERGENCY_STOP      "emerg_stop"
#define SYS_KEY_AUTO_DISARM_TIMEOUT "auto_disarm"
#define SYS_KEY_SAFETY_VOLTAGE_MIN  "volt_min"
#define SYS_KEY_TEMP_LIMIT_MAX      "temp_max"
#define SYS_KEY_MOTION_TIMEOUT      "motion_timeout"
#define SYS_KEY_STARTUP_DELAY       "startup_delay"
#define SYS_KEY_MAX_CONTROL_FREQ    "max_ctrl_freq"
#define SYS_KEY_ROBOT_ID            "robot_id"
#define SYS_KEY_ROBOT_NAME          "robot_name"
#define SYS_KEY_CONFIG_VERSION      "config_ver"
#define SYS_KEY_CONTROLLER_TYPE     "ctrl_type"

#define JOINT_KEY_OFFSET_FORMAT  "l%d_%s_off"
#define JOINT_KEY_INVERT_FORMAT  "l%d_%s_inv"
#define JOINT_KEY_MIN_FORMAT     "l%d_%s_min"
#define JOINT_KEY_MAX_FORMAT     "l%d_%s_max"
#define JOINT_KEY_PWM_MIN_FORMAT "l%d_%s_pmin"
#define JOINT_KEY_PWM_MAX_FORMAT "l%d_%s_pmax"
#define JOINT_KEY_NEUTRAL_FORMAT "l%d_%s_neut"

#define LEG_GEOM_KEY_LEN_COXA_FORMAT   "l%d_lc"
#define LEG_GEOM_KEY_LEN_FEMUR_FORMAT  "l%d_lf"
#define LEG_GEOM_KEY_LEN_TIBIA_FORMAT  "l%d_lt"
#define LEG_GEOM_KEY_MOUNT_X_FORMAT    "l%d_mx"
#define LEG_GEOM_KEY_MOUNT_Y_FORMAT    "l%d_my"
#define LEG_GEOM_KEY_MOUNT_Z_FORMAT    "l%d_mz"
#define LEG_GEOM_KEY_MOUNT_YAW_FORMAT  "l%d_myaw"
#define LEG_GEOM_KEY_STANCE_OUT_FORMAT "l%d_so"
#define LEG_GEOM_KEY_STANCE_FWD_FORMAT "l%d_sf"

#define MOT_KEY_MAX_VEL_FORMAT   "mv_%s"
#define MOT_KEY_MAX_ACC_FORMAT   "ma_%s"
#define MOT_KEY_MAX_JERK_FORMAT  "mj_%s"
#define MOT_KEY_VEL_FILT_ALPHA   "vf_a"
#define MOT_KEY_ACC_FILT_ALPHA   "af_a"
#define MOT_KEY_LEG_VF_ALPHA     "lvf_a"
#define MOT_KEY_BODY_VF_ALPHA    "bvf_a"
#define MOT_KEY_BODY_P_ALPHA     "bp_a"
#define MOT_KEY_BODY_R_ALPHA     "br_a"
#define MOT_KEY_FRONT_BACK_DIST  "fb_d"
#define MOT_KEY_LEFT_RIGHT_DIST  "lr_d"
#define MOT_KEY_MAX_LEG_VEL      "ml_v"
#define MOT_KEY_MAX_BODY_VEL     "mb_v"
#define MOT_KEY_MAX_ANG_VEL      "ma_v"
#define MOT_KEY_MIN_DT           "min_dt"
#define MOT_KEY_MAX_DT           "max_dt"
#define MOT_KEY_BODY_OFF_X       "bo_x"
#define MOT_KEY_BODY_OFF_Y       "bo_y"
#define MOT_KEY_BODY_OFF_Z       "bo_z"

#define CTRL_KEY_DRIVER_TYPE     "drv_t"
#define CTRL_KEY_TASK_STACK      "tsk_stk"
#define CTRL_KEY_TASK_PRIORITY   "tsk_pri"
#define CTRL_KEY_FLYSKY_UART     "fly_u"
#define CTRL_KEY_FLYSKY_TX       "fly_tx"
#define CTRL_KEY_FLYSKY_RX       "fly_rx"
#define CTRL_KEY_FLYSKY_RTS      "fly_rts"
#define CTRL_KEY_FLYSKY_CTS      "fly_cts"
#define CTRL_KEY_FLYSKY_BAUD     "fly_baud"

static const char* JOINT_NAMES[] = { "c", "f", "t" };

static esp_err_t nvs_set_float_blob(nvs_handle_t handle, const char* key, float value) {
    return nvs_set_blob(handle, key, &value, sizeof(float));
}

static esp_err_t nvs_get_float_blob(nvs_handle_t handle, const char* key, float* value_out) {
    size_t required_size = sizeof(float);
    return nvs_get_blob(handle, key, value_out, &required_size);
}

esp_err_t config_domain_system_write_defaults_to_nvs(
    nvs_handle_t handle,
    uint16_t schema_version
) {
    ESP_LOGI(TAG, "Initializing system namespace defaults to NVS");

    ESP_ERROR_CHECK(nvs_set_u8(handle, SYS_KEY_EMERGENCY_STOP, 1));
    ESP_ERROR_CHECK(nvs_set_u32(handle, SYS_KEY_AUTO_DISARM_TIMEOUT, 30));
    ESP_ERROR_CHECK(nvs_set_u32(handle, SYS_KEY_MOTION_TIMEOUT, 1000));
    ESP_ERROR_CHECK(nvs_set_u32(handle, SYS_KEY_STARTUP_DELAY, 2000));
    ESP_ERROR_CHECK(nvs_set_u32(handle, SYS_KEY_MAX_CONTROL_FREQ, 100));

    float default_voltage = 6.5f;
    float default_temp = 80.0f;
    ESP_ERROR_CHECK(nvs_set_blob(handle, SYS_KEY_SAFETY_VOLTAGE_MIN, &default_voltage, sizeof(float)));
    ESP_ERROR_CHECK(nvs_set_blob(handle, SYS_KEY_TEMP_LIMIT_MAX, &default_temp, sizeof(float)));

    char robot_id[32];
    uint8_t mac[6];
    esp_err_t err = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (err == ESP_OK) {
        snprintf(robot_id, sizeof(robot_id), "HEXAPOD_%02X%02X%02X", mac[3], mac[4], mac[5]);
    } else {
        strcpy(robot_id, "HEXAPOD_DEFAULT");
    }

    ESP_ERROR_CHECK(nvs_set_str(handle, SYS_KEY_ROBOT_ID, robot_id));
    ESP_ERROR_CHECK(nvs_set_str(handle, SYS_KEY_ROBOT_NAME, "My Hexapod Robot"));
    ESP_ERROR_CHECK(nvs_set_u16(handle, SYS_KEY_CONFIG_VERSION, schema_version));
    ESP_ERROR_CHECK(nvs_set_u8(handle, SYS_KEY_CONTROLLER_TYPE, (uint8_t)CONTROLLER_DRIVER_FLYSKY_IBUS));

    ESP_ERROR_CHECK(nvs_commit(handle));
    ESP_LOGI(TAG, "System defaults written to NVS");

    return ESP_OK;
}

esp_err_t config_domain_joint_cal_write_defaults_to_nvs(nvs_handle_t handle) {
    char key[32];

    ESP_LOGI(TAG, "Initializing joint calibration namespace defaults to NVS");

    for (int leg = 0; leg < NUM_LEGS; leg++) {
        for (int joint = 0; joint < NUM_JOINTS_PER_LEG; joint++) {
            const char* joint_name = JOINT_NAMES[joint];

            joint_calib_t default_calib;
            config_domain_fill_joint_default(joint, &default_calib);

            snprintf(key, sizeof(key), JOINT_KEY_OFFSET_FORMAT, leg, joint_name);
            ESP_ERROR_CHECK(nvs_set_blob(handle, key, &default_calib.zero_offset_rad, sizeof(float)));

            snprintf(key, sizeof(key), JOINT_KEY_INVERT_FORMAT, leg, joint_name);
            ESP_ERROR_CHECK(nvs_set_i8(handle, key, default_calib.invert_sign));

            snprintf(key, sizeof(key), JOINT_KEY_MIN_FORMAT, leg, joint_name);
            ESP_ERROR_CHECK(nvs_set_blob(handle, key, &default_calib.min_rad, sizeof(float)));

            snprintf(key, sizeof(key), JOINT_KEY_MAX_FORMAT, leg, joint_name);
            ESP_ERROR_CHECK(nvs_set_blob(handle, key, &default_calib.max_rad, sizeof(float)));

            snprintf(key, sizeof(key), JOINT_KEY_PWM_MIN_FORMAT, leg, joint_name);
            ESP_ERROR_CHECK(nvs_set_i32(handle, key, default_calib.pwm_min_us));

            snprintf(key, sizeof(key), JOINT_KEY_PWM_MAX_FORMAT, leg, joint_name);
            ESP_ERROR_CHECK(nvs_set_i32(handle, key, default_calib.pwm_max_us));

            snprintf(key, sizeof(key), JOINT_KEY_NEUTRAL_FORMAT, leg, joint_name);
            ESP_ERROR_CHECK(nvs_set_i32(handle, key, default_calib.neutral_us));
        }
    }

    ESP_ERROR_CHECK(nvs_commit(handle));
    ESP_LOGI(TAG, "Joint calibration defaults written to NVS");

    return ESP_OK;
}

esp_err_t config_domain_leg_geometry_write_defaults_to_nvs(nvs_handle_t handle) {
    char key[32];
    leg_geometry_config_t defaults;

    config_load_leg_geometry_defaults(&defaults);

    ESP_LOGI(TAG, "Initializing leg geometry namespace defaults to NVS");

    for (int leg = 0; leg < NUM_LEGS; leg++) {
        snprintf(key, sizeof(key), LEG_GEOM_KEY_LEN_COXA_FORMAT, leg);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, defaults.len_coxa[leg]));

        snprintf(key, sizeof(key), LEG_GEOM_KEY_LEN_FEMUR_FORMAT, leg);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, defaults.len_femur[leg]));

        snprintf(key, sizeof(key), LEG_GEOM_KEY_LEN_TIBIA_FORMAT, leg);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, defaults.len_tibia[leg]));

        snprintf(key, sizeof(key), LEG_GEOM_KEY_MOUNT_X_FORMAT, leg);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, defaults.mount_x[leg]));

        snprintf(key, sizeof(key), LEG_GEOM_KEY_MOUNT_Y_FORMAT, leg);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, defaults.mount_y[leg]));

        snprintf(key, sizeof(key), LEG_GEOM_KEY_MOUNT_Z_FORMAT, leg);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, defaults.mount_z[leg]));

        snprintf(key, sizeof(key), LEG_GEOM_KEY_MOUNT_YAW_FORMAT, leg);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, defaults.mount_yaw[leg]));

        snprintf(key, sizeof(key), LEG_GEOM_KEY_STANCE_OUT_FORMAT, leg);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, defaults.stance_out[leg]));

        snprintf(key, sizeof(key), LEG_GEOM_KEY_STANCE_FWD_FORMAT, leg);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, defaults.stance_fwd[leg]));
    }

    ESP_ERROR_CHECK(nvs_commit(handle));
    ESP_LOGI(TAG, "Leg geometry defaults written to NVS");

    return ESP_OK;
}

esp_err_t config_domain_motion_limits_write_defaults_to_nvs(nvs_handle_t handle) {
    char key[32];
    motion_limits_config_t defaults;

    config_load_motion_limits_defaults(&defaults);

    ESP_LOGI(TAG, "Initializing motion limits namespace defaults to NVS");

    for (int joint = 0; joint < NUM_JOINTS_PER_LEG; joint++) {
        const char* joint_name = JOINT_NAMES[joint];

        snprintf(key, sizeof(key), MOT_KEY_MAX_VEL_FORMAT, joint_name);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, defaults.max_velocity[joint]));

        snprintf(key, sizeof(key), MOT_KEY_MAX_ACC_FORMAT, joint_name);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, defaults.max_acceleration[joint]));

        snprintf(key, sizeof(key), MOT_KEY_MAX_JERK_FORMAT, joint_name);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, defaults.max_jerk[joint]));
    }

    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_VEL_FILT_ALPHA, defaults.velocity_filter_alpha));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_ACC_FILT_ALPHA, defaults.accel_filter_alpha));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_LEG_VF_ALPHA, defaults.leg_velocity_filter_alpha));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_BODY_VF_ALPHA, defaults.body_velocity_filter_alpha));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_BODY_P_ALPHA, defaults.body_pitch_filter_alpha));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_BODY_R_ALPHA, defaults.body_roll_filter_alpha));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_FRONT_BACK_DIST, defaults.front_to_back_distance));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_LEFT_RIGHT_DIST, defaults.left_to_right_distance));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_MAX_LEG_VEL, defaults.max_leg_velocity));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_MAX_BODY_VEL, defaults.max_body_velocity));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_MAX_ANG_VEL, defaults.max_angular_velocity));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_MIN_DT, defaults.min_dt));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_MAX_DT, defaults.max_dt));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_BODY_OFF_X, defaults.body_offset_x));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_BODY_OFF_Y, defaults.body_offset_y));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_BODY_OFF_Z, defaults.body_offset_z));

    ESP_ERROR_CHECK(nvs_commit(handle));
    ESP_LOGI(TAG, "Motion limits defaults written to NVS");

    return ESP_OK;
}

esp_err_t config_domain_controller_write_defaults_to_nvs(nvs_handle_t handle) {
    controller_config_namespace_t defaults;

    config_load_controller_defaults(&defaults);

    ESP_LOGI(TAG, "Initializing controller namespace defaults to NVS");

    ESP_ERROR_CHECK(nvs_set_u8(handle, CTRL_KEY_DRIVER_TYPE, (uint8_t)defaults.driver_type));
    ESP_ERROR_CHECK(nvs_set_u32(handle, CTRL_KEY_TASK_STACK, defaults.task_stack));
    ESP_ERROR_CHECK(nvs_set_u32(handle, CTRL_KEY_TASK_PRIORITY, defaults.task_priority));

    ESP_ERROR_CHECK(nvs_set_i32(handle, CTRL_KEY_FLYSKY_UART, defaults.flysky_uart_port));
    ESP_ERROR_CHECK(nvs_set_i32(handle, CTRL_KEY_FLYSKY_TX, defaults.flysky_tx_gpio));
    ESP_ERROR_CHECK(nvs_set_i32(handle, CTRL_KEY_FLYSKY_RX, defaults.flysky_rx_gpio));
    ESP_ERROR_CHECK(nvs_set_i32(handle, CTRL_KEY_FLYSKY_RTS, defaults.flysky_rts_gpio));
    ESP_ERROR_CHECK(nvs_set_i32(handle, CTRL_KEY_FLYSKY_CTS, defaults.flysky_cts_gpio));
    ESP_ERROR_CHECK(nvs_set_i32(handle, CTRL_KEY_FLYSKY_BAUD, defaults.flysky_baud_rate));

    ESP_ERROR_CHECK(nvs_commit(handle));
    ESP_LOGI(TAG, "Controller defaults written to NVS");

    return ESP_OK;
}

esp_err_t config_domain_system_load_from_nvs(
    nvs_handle_t handle,
    system_config_t* config,
    uint16_t fallback_schema_version,
    controller_driver_type_e fallback_controller_type
) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err;
    size_t required_size = 0;

    ESP_LOGI(TAG, "Loading system configuration from NVS (read-only)");

    uint8_t temp_bool = 0;
    err = nvs_get_u8(handle, SYS_KEY_EMERGENCY_STOP, &temp_bool);
    if (err == ESP_OK) {
        config->emergency_stop_enabled = (temp_bool != 0);
    } else if (err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to read emergency_stop, using default: %s", esp_err_to_name(err));
    }

    err = nvs_get_u32(handle, SYS_KEY_AUTO_DISARM_TIMEOUT, &config->auto_disarm_timeout);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to read auto_disarm_timeout: %s", esp_err_to_name(err));
    }

    err = nvs_get_u32(handle, SYS_KEY_MOTION_TIMEOUT, &config->motion_timeout_ms);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to read motion_timeout: %s", esp_err_to_name(err));
    }

    err = nvs_get_u32(handle, SYS_KEY_STARTUP_DELAY, &config->startup_delay_ms);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to read startup_delay: %s", esp_err_to_name(err));
    }

    err = nvs_get_u32(handle, SYS_KEY_MAX_CONTROL_FREQ, &config->max_control_frequency);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to read max_control_freq: %s", esp_err_to_name(err));
    }

    required_size = sizeof(float);
    err = nvs_get_blob(handle, SYS_KEY_SAFETY_VOLTAGE_MIN, &config->safety_voltage_min, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to read safety_voltage_min: %s", esp_err_to_name(err));
    }

    required_size = sizeof(float);
    err = nvs_get_blob(handle, SYS_KEY_TEMP_LIMIT_MAX, &config->temperature_limit_max, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to read temp_limit_max: %s", esp_err_to_name(err));
    }

    required_size = sizeof(config->robot_id);
    err = nvs_get_str(handle, SYS_KEY_ROBOT_ID, config->robot_id, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to read robot_id: %s", esp_err_to_name(err));
    }

    required_size = sizeof(config->robot_name);
    err = nvs_get_str(handle, SYS_KEY_ROBOT_NAME, config->robot_name, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to read robot_name: %s", esp_err_to_name(err));
    }

    uint16_t namespace_version = 0;
    err = nvs_get_u16(handle, SYS_KEY_CONFIG_VERSION, &namespace_version);
    if (err == ESP_OK) {
        config->config_version = namespace_version;
    } else {
        ESP_LOGW(TAG, "Failed to read namespace config version: %s", esp_err_to_name(err));
        config->config_version = fallback_schema_version;
    }

    uint8_t ctrl_type = 0;
    err = nvs_get_u8(handle, SYS_KEY_CONTROLLER_TYPE, &ctrl_type);
    if (err == ESP_OK) {
        config->controller_type = (controller_driver_type_e)ctrl_type;
    } else {
        ESP_LOGW(TAG, "Failed to read controller_type, using default: %s", esp_err_to_name(err));
        config->controller_type = fallback_controller_type;
    }

    return ESP_OK;
}

esp_err_t config_domain_joint_cal_load_from_nvs(
    nvs_handle_t handle,
    joint_calib_config_t* config
) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err;
    char key[32];

    ESP_LOGI(TAG, "Loading joint calibration configuration from NVS (read-only)");

    for (int leg = 0; leg < NUM_LEGS; leg++) {
        for (int joint = 0; joint < NUM_JOINTS_PER_LEG; joint++) {
            const char* joint_name = JOINT_NAMES[joint];
            joint_calib_t* calib = &config->joints[leg][joint];

            snprintf(key, sizeof(key), JOINT_KEY_OFFSET_FORMAT, leg, joint_name);
            size_t required_size = sizeof(float);
            err = nvs_get_blob(handle, key, &calib->zero_offset_rad, &required_size);
            if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
                ESP_LOGW(TAG, "Failed to load offset for leg %d joint %s: %s", leg, joint_name, esp_err_to_name(err));
            }

            snprintf(key, sizeof(key), JOINT_KEY_INVERT_FORMAT, leg, joint_name);
            err = nvs_get_i8(handle, key, &calib->invert_sign);
            if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
                ESP_LOGW(TAG, "Failed to load invert for leg %d joint %s: %s", leg, joint_name, esp_err_to_name(err));
            }

            snprintf(key, sizeof(key), JOINT_KEY_MIN_FORMAT, leg, joint_name);
            required_size = sizeof(float);
            err = nvs_get_blob(handle, key, &calib->min_rad, &required_size);
            if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
                ESP_LOGW(TAG, "Failed to load min limit for leg %d joint %s: %s", leg, joint_name, esp_err_to_name(err));
            }

            snprintf(key, sizeof(key), JOINT_KEY_MAX_FORMAT, leg, joint_name);
            required_size = sizeof(float);
            err = nvs_get_blob(handle, key, &calib->max_rad, &required_size);
            if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
                ESP_LOGW(TAG, "Failed to load max limit for leg %d joint %s: %s", leg, joint_name, esp_err_to_name(err));
            }

            snprintf(key, sizeof(key), JOINT_KEY_PWM_MIN_FORMAT, leg, joint_name);
            err = nvs_get_i32(handle, key, &calib->pwm_min_us);
            if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
                ESP_LOGW(TAG, "Failed to load PWM min for leg %d joint %s: %s", leg, joint_name, esp_err_to_name(err));
            }

            snprintf(key, sizeof(key), JOINT_KEY_PWM_MAX_FORMAT, leg, joint_name);
            err = nvs_get_i32(handle, key, &calib->pwm_max_us);
            if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
                ESP_LOGW(TAG, "Failed to load PWM max for leg %d joint %s: %s", leg, joint_name, esp_err_to_name(err));
            }

            snprintf(key, sizeof(key), JOINT_KEY_NEUTRAL_FORMAT, leg, joint_name);
            err = nvs_get_i32(handle, key, &calib->neutral_us);
            if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
                ESP_LOGW(TAG, "Failed to load PWM neutral for leg %d joint %s: %s", leg, joint_name, esp_err_to_name(err));
            }
        }
    }

    return ESP_OK;
}

esp_err_t config_domain_leg_geometry_load_from_nvs(
    nvs_handle_t handle,
    leg_geometry_config_t* config
) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err;
    char key[32];

    ESP_LOGI(TAG, "Loading leg geometry configuration from NVS (read-only)");

    for (int leg = 0; leg < NUM_LEGS; leg++) {
        snprintf(key, sizeof(key), LEG_GEOM_KEY_LEN_COXA_FORMAT, leg);
        err = nvs_get_float_blob(handle, key, &config->len_coxa[leg]);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "Failed to load len_coxa for leg %d: %s", leg, esp_err_to_name(err));
        }

        snprintf(key, sizeof(key), LEG_GEOM_KEY_LEN_FEMUR_FORMAT, leg);
        err = nvs_get_float_blob(handle, key, &config->len_femur[leg]);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "Failed to load len_femur for leg %d: %s", leg, esp_err_to_name(err));
        }

        snprintf(key, sizeof(key), LEG_GEOM_KEY_LEN_TIBIA_FORMAT, leg);
        err = nvs_get_float_blob(handle, key, &config->len_tibia[leg]);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "Failed to load len_tibia for leg %d: %s", leg, esp_err_to_name(err));
        }

        snprintf(key, sizeof(key), LEG_GEOM_KEY_MOUNT_X_FORMAT, leg);
        err = nvs_get_float_blob(handle, key, &config->mount_x[leg]);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "Failed to load mount_x for leg %d: %s", leg, esp_err_to_name(err));
        }

        snprintf(key, sizeof(key), LEG_GEOM_KEY_MOUNT_Y_FORMAT, leg);
        err = nvs_get_float_blob(handle, key, &config->mount_y[leg]);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "Failed to load mount_y for leg %d: %s", leg, esp_err_to_name(err));
        }

        snprintf(key, sizeof(key), LEG_GEOM_KEY_MOUNT_Z_FORMAT, leg);
        err = nvs_get_float_blob(handle, key, &config->mount_z[leg]);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "Failed to load mount_z for leg %d: %s", leg, esp_err_to_name(err));
        }

        snprintf(key, sizeof(key), LEG_GEOM_KEY_MOUNT_YAW_FORMAT, leg);
        err = nvs_get_float_blob(handle, key, &config->mount_yaw[leg]);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "Failed to load mount_yaw for leg %d: %s", leg, esp_err_to_name(err));
        }

        snprintf(key, sizeof(key), LEG_GEOM_KEY_STANCE_OUT_FORMAT, leg);
        err = nvs_get_float_blob(handle, key, &config->stance_out[leg]);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "Failed to load stance_out for leg %d: %s", leg, esp_err_to_name(err));
        }

        snprintf(key, sizeof(key), LEG_GEOM_KEY_STANCE_FWD_FORMAT, leg);
        err = nvs_get_float_blob(handle, key, &config->stance_fwd[leg]);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "Failed to load stance_fwd for leg %d: %s", leg, esp_err_to_name(err));
        }
    }

    return ESP_OK;
}

esp_err_t config_domain_motion_limits_load_from_nvs(
    nvs_handle_t handle,
    motion_limits_config_t* config
) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err;
    char key[32];

    ESP_LOGI(TAG, "Loading motion limits configuration from NVS (read-only)");

    for (int joint = 0; joint < NUM_JOINTS_PER_LEG; joint++) {
        const char* joint_name = JOINT_NAMES[joint];

        snprintf(key, sizeof(key), MOT_KEY_MAX_VEL_FORMAT, joint_name);
        err = nvs_get_float_blob(handle, key, &config->max_velocity[joint]);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "Failed to load max_velocity[%d]: %s", joint, esp_err_to_name(err));
        }

        snprintf(key, sizeof(key), MOT_KEY_MAX_ACC_FORMAT, joint_name);
        err = nvs_get_float_blob(handle, key, &config->max_acceleration[joint]);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "Failed to load max_acceleration[%d]: %s", joint, esp_err_to_name(err));
        }

        snprintf(key, sizeof(key), MOT_KEY_MAX_JERK_FORMAT, joint_name);
        err = nvs_get_float_blob(handle, key, &config->max_jerk[joint]);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "Failed to load max_jerk[%d]: %s", joint, esp_err_to_name(err));
        }
    }

    err = nvs_get_float_blob(handle, MOT_KEY_VEL_FILT_ALPHA, &config->velocity_filter_alpha);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load velocity_filter_alpha: %s", esp_err_to_name(err));
    }

    err = nvs_get_float_blob(handle, MOT_KEY_ACC_FILT_ALPHA, &config->accel_filter_alpha);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load accel_filter_alpha: %s", esp_err_to_name(err));
    }

    err = nvs_get_float_blob(handle, MOT_KEY_LEG_VF_ALPHA, &config->leg_velocity_filter_alpha);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load leg_velocity_filter_alpha: %s", esp_err_to_name(err));
    }

    err = nvs_get_float_blob(handle, MOT_KEY_BODY_VF_ALPHA, &config->body_velocity_filter_alpha);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load body_velocity_filter_alpha: %s", esp_err_to_name(err));
    }

    err = nvs_get_float_blob(handle, MOT_KEY_BODY_P_ALPHA, &config->body_pitch_filter_alpha);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load body_pitch_filter_alpha: %s", esp_err_to_name(err));
    }

    err = nvs_get_float_blob(handle, MOT_KEY_BODY_R_ALPHA, &config->body_roll_filter_alpha);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load body_roll_filter_alpha: %s", esp_err_to_name(err));
    }

    err = nvs_get_float_blob(handle, MOT_KEY_FRONT_BACK_DIST, &config->front_to_back_distance);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load front_to_back_distance: %s", esp_err_to_name(err));
    }

    err = nvs_get_float_blob(handle, MOT_KEY_LEFT_RIGHT_DIST, &config->left_to_right_distance);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load left_to_right_distance: %s", esp_err_to_name(err));
    }

    err = nvs_get_float_blob(handle, MOT_KEY_MAX_LEG_VEL, &config->max_leg_velocity);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load max_leg_velocity: %s", esp_err_to_name(err));
    }

    err = nvs_get_float_blob(handle, MOT_KEY_MAX_BODY_VEL, &config->max_body_velocity);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load max_body_velocity: %s", esp_err_to_name(err));
    }

    err = nvs_get_float_blob(handle, MOT_KEY_MAX_ANG_VEL, &config->max_angular_velocity);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load max_angular_velocity: %s", esp_err_to_name(err));
    }

    err = nvs_get_float_blob(handle, MOT_KEY_MIN_DT, &config->min_dt);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load min_dt: %s", esp_err_to_name(err));
    }

    err = nvs_get_float_blob(handle, MOT_KEY_MAX_DT, &config->max_dt);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load max_dt: %s", esp_err_to_name(err));
    }

    err = nvs_get_float_blob(handle, MOT_KEY_BODY_OFF_X, &config->body_offset_x);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load body_offset_x: %s", esp_err_to_name(err));
    }

    err = nvs_get_float_blob(handle, MOT_KEY_BODY_OFF_Y, &config->body_offset_y);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load body_offset_y: %s", esp_err_to_name(err));
    }

    err = nvs_get_float_blob(handle, MOT_KEY_BODY_OFF_Z, &config->body_offset_z);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load body_offset_z: %s", esp_err_to_name(err));
    }

    return ESP_OK;
}

esp_err_t config_domain_controller_load_from_nvs(
    nvs_handle_t handle,
    controller_config_namespace_t* config
) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err;
    uint8_t driver_type = 0;

    ESP_LOGI(TAG, "Loading controller configuration from NVS (read-only)");

    err = nvs_get_u8(handle, CTRL_KEY_DRIVER_TYPE, &driver_type);
    if (err == ESP_OK) {
        config->driver_type = (controller_driver_type_e)driver_type;
    } else if (err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load driver_type: %s", esp_err_to_name(err));
    }

    err = nvs_get_u32(handle, CTRL_KEY_TASK_STACK, &config->task_stack);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load task_stack: %s", esp_err_to_name(err));
    }

    err = nvs_get_u32(handle, CTRL_KEY_TASK_PRIORITY, &config->task_priority);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load task_priority: %s", esp_err_to_name(err));
    }

    err = nvs_get_i32(handle, CTRL_KEY_FLYSKY_UART, &config->flysky_uart_port);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load flysky_uart_port: %s", esp_err_to_name(err));
    }

    err = nvs_get_i32(handle, CTRL_KEY_FLYSKY_TX, &config->flysky_tx_gpio);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load flysky_tx_gpio: %s", esp_err_to_name(err));
    }

    err = nvs_get_i32(handle, CTRL_KEY_FLYSKY_RX, &config->flysky_rx_gpio);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load flysky_rx_gpio: %s", esp_err_to_name(err));
    }

    err = nvs_get_i32(handle, CTRL_KEY_FLYSKY_RTS, &config->flysky_rts_gpio);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load flysky_rts_gpio: %s", esp_err_to_name(err));
    }

    err = nvs_get_i32(handle, CTRL_KEY_FLYSKY_CTS, &config->flysky_cts_gpio);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load flysky_cts_gpio: %s", esp_err_to_name(err));
    }

    err = nvs_get_i32(handle, CTRL_KEY_FLYSKY_BAUD, &config->flysky_baud_rate);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to load flysky_baud_rate: %s", esp_err_to_name(err));
    }

    return ESP_OK;
}

esp_err_t config_domain_system_save_to_nvs(
    nvs_handle_t handle,
    const system_config_t* config
) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t temp_bool = config->emergency_stop_enabled ? 1 : 0;
    ESP_ERROR_CHECK(nvs_set_u8(handle, SYS_KEY_EMERGENCY_STOP, temp_bool));
    ESP_ERROR_CHECK(nvs_set_u32(handle, SYS_KEY_AUTO_DISARM_TIMEOUT, config->auto_disarm_timeout));
    ESP_ERROR_CHECK(nvs_set_u32(handle, SYS_KEY_MOTION_TIMEOUT, config->motion_timeout_ms));
    ESP_ERROR_CHECK(nvs_set_u32(handle, SYS_KEY_STARTUP_DELAY, config->startup_delay_ms));
    ESP_ERROR_CHECK(nvs_set_u32(handle, SYS_KEY_MAX_CONTROL_FREQ, config->max_control_frequency));

    ESP_ERROR_CHECK(nvs_set_blob(handle, SYS_KEY_SAFETY_VOLTAGE_MIN, &config->safety_voltage_min, sizeof(float)));
    ESP_ERROR_CHECK(nvs_set_blob(handle, SYS_KEY_TEMP_LIMIT_MAX, &config->temperature_limit_max, sizeof(float)));

    ESP_ERROR_CHECK(nvs_set_str(handle, SYS_KEY_ROBOT_ID, config->robot_id));
    ESP_ERROR_CHECK(nvs_set_str(handle, SYS_KEY_ROBOT_NAME, config->robot_name));

    ESP_ERROR_CHECK(nvs_set_u16(handle, SYS_KEY_CONFIG_VERSION, config->config_version));
    ESP_ERROR_CHECK(nvs_set_u8(handle, SYS_KEY_CONTROLLER_TYPE, (uint8_t)config->controller_type));

    return nvs_commit(handle);
}

esp_err_t config_domain_joint_cal_save_to_nvs(
    nvs_handle_t handle,
    const joint_calib_config_t* config
) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    char key[32];

    for (int leg = 0; leg < NUM_LEGS; leg++) {
        for (int joint = 0; joint < NUM_JOINTS_PER_LEG; joint++) {
            const char* joint_name = JOINT_NAMES[joint];
            const joint_calib_t* calib = &config->joints[leg][joint];

            snprintf(key, sizeof(key), JOINT_KEY_OFFSET_FORMAT, leg, joint_name);
            ESP_ERROR_CHECK(nvs_set_blob(handle, key, &calib->zero_offset_rad, sizeof(float)));

            snprintf(key, sizeof(key), JOINT_KEY_INVERT_FORMAT, leg, joint_name);
            ESP_ERROR_CHECK(nvs_set_i8(handle, key, calib->invert_sign));

            snprintf(key, sizeof(key), JOINT_KEY_MIN_FORMAT, leg, joint_name);
            ESP_ERROR_CHECK(nvs_set_blob(handle, key, &calib->min_rad, sizeof(float)));

            snprintf(key, sizeof(key), JOINT_KEY_MAX_FORMAT, leg, joint_name);
            ESP_ERROR_CHECK(nvs_set_blob(handle, key, &calib->max_rad, sizeof(float)));

            snprintf(key, sizeof(key), JOINT_KEY_PWM_MIN_FORMAT, leg, joint_name);
            ESP_ERROR_CHECK(nvs_set_i32(handle, key, calib->pwm_min_us));

            snprintf(key, sizeof(key), JOINT_KEY_PWM_MAX_FORMAT, leg, joint_name);
            ESP_ERROR_CHECK(nvs_set_i32(handle, key, calib->pwm_max_us));

            snprintf(key, sizeof(key), JOINT_KEY_NEUTRAL_FORMAT, leg, joint_name);
            ESP_ERROR_CHECK(nvs_set_i32(handle, key, calib->neutral_us));
        }
    }

    return nvs_commit(handle);
}

esp_err_t config_domain_leg_geometry_save_to_nvs(
    nvs_handle_t handle,
    const leg_geometry_config_t* config
) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    char key[32];

    for (int leg = 0; leg < NUM_LEGS; leg++) {
        snprintf(key, sizeof(key), LEG_GEOM_KEY_LEN_COXA_FORMAT, leg);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, config->len_coxa[leg]));

        snprintf(key, sizeof(key), LEG_GEOM_KEY_LEN_FEMUR_FORMAT, leg);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, config->len_femur[leg]));

        snprintf(key, sizeof(key), LEG_GEOM_KEY_LEN_TIBIA_FORMAT, leg);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, config->len_tibia[leg]));

        snprintf(key, sizeof(key), LEG_GEOM_KEY_MOUNT_X_FORMAT, leg);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, config->mount_x[leg]));

        snprintf(key, sizeof(key), LEG_GEOM_KEY_MOUNT_Y_FORMAT, leg);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, config->mount_y[leg]));

        snprintf(key, sizeof(key), LEG_GEOM_KEY_MOUNT_Z_FORMAT, leg);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, config->mount_z[leg]));

        snprintf(key, sizeof(key), LEG_GEOM_KEY_MOUNT_YAW_FORMAT, leg);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, config->mount_yaw[leg]));

        snprintf(key, sizeof(key), LEG_GEOM_KEY_STANCE_OUT_FORMAT, leg);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, config->stance_out[leg]));

        snprintf(key, sizeof(key), LEG_GEOM_KEY_STANCE_FWD_FORMAT, leg);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, config->stance_fwd[leg]));
    }

    return nvs_commit(handle);
}

esp_err_t config_domain_motion_limits_save_to_nvs(
    nvs_handle_t handle,
    const motion_limits_config_t* config
) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    char key[32];

    for (int joint = 0; joint < NUM_JOINTS_PER_LEG; joint++) {
        const char* joint_name = JOINT_NAMES[joint];

        snprintf(key, sizeof(key), MOT_KEY_MAX_VEL_FORMAT, joint_name);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, config->max_velocity[joint]));

        snprintf(key, sizeof(key), MOT_KEY_MAX_ACC_FORMAT, joint_name);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, config->max_acceleration[joint]));

        snprintf(key, sizeof(key), MOT_KEY_MAX_JERK_FORMAT, joint_name);
        ESP_ERROR_CHECK(nvs_set_float_blob(handle, key, config->max_jerk[joint]));
    }

    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_VEL_FILT_ALPHA, config->velocity_filter_alpha));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_ACC_FILT_ALPHA, config->accel_filter_alpha));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_LEG_VF_ALPHA, config->leg_velocity_filter_alpha));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_BODY_VF_ALPHA, config->body_velocity_filter_alpha));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_BODY_P_ALPHA, config->body_pitch_filter_alpha));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_BODY_R_ALPHA, config->body_roll_filter_alpha));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_FRONT_BACK_DIST, config->front_to_back_distance));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_LEFT_RIGHT_DIST, config->left_to_right_distance));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_MAX_LEG_VEL, config->max_leg_velocity));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_MAX_BODY_VEL, config->max_body_velocity));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_MAX_ANG_VEL, config->max_angular_velocity));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_MIN_DT, config->min_dt));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_MAX_DT, config->max_dt));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_BODY_OFF_X, config->body_offset_x));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_BODY_OFF_Y, config->body_offset_y));
    ESP_ERROR_CHECK(nvs_set_float_blob(handle, MOT_KEY_BODY_OFF_Z, config->body_offset_z));

    return nvs_commit(handle);
}

esp_err_t config_domain_controller_save_to_nvs(
    nvs_handle_t handle,
    const controller_config_namespace_t* config
) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_ERROR_CHECK(nvs_set_u8(handle, CTRL_KEY_DRIVER_TYPE, (uint8_t)config->driver_type));
    ESP_ERROR_CHECK(nvs_set_u32(handle, CTRL_KEY_TASK_STACK, config->task_stack));
    ESP_ERROR_CHECK(nvs_set_u32(handle, CTRL_KEY_TASK_PRIORITY, config->task_priority));

    ESP_ERROR_CHECK(nvs_set_i32(handle, CTRL_KEY_FLYSKY_UART, config->flysky_uart_port));
    ESP_ERROR_CHECK(nvs_set_i32(handle, CTRL_KEY_FLYSKY_TX, config->flysky_tx_gpio));
    ESP_ERROR_CHECK(nvs_set_i32(handle, CTRL_KEY_FLYSKY_RX, config->flysky_rx_gpio));
    ESP_ERROR_CHECK(nvs_set_i32(handle, CTRL_KEY_FLYSKY_RTS, config->flysky_rts_gpio));
    ESP_ERROR_CHECK(nvs_set_i32(handle, CTRL_KEY_FLYSKY_CTS, config->flysky_cts_gpio));
    ESP_ERROR_CHECK(nvs_set_i32(handle, CTRL_KEY_FLYSKY_BAUD, config->flysky_baud_rate));

    return nvs_commit(handle);
}
