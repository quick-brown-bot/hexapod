#include "config_domain_defaults.h"

#include "esp_mac.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

#define CONFIG_SCHEMA_VERSION_DEFAULT 1

void config_domain_fill_joint_default(int joint, joint_calib_t* calib) {
    if (!calib || joint < 0 || joint >= NUM_JOINTS_PER_LEG) {
        return;
    }

    calib->zero_offset_rad = 0.0f;
    calib->invert_sign = (joint == 0) ? 1 : -1;
    calib->min_rad = -1.57f;
    calib->max_rad = 1.57f;
    calib->pwm_min_us = 1000;
    calib->pwm_max_us = 2000;
    calib->neutral_us = 1500;
}

void config_load_system_defaults(system_config_t* config) {
    if (!config) {
        return;
    }

    memset(config, 0, sizeof(system_config_t));

    config->emergency_stop_enabled = true;
    config->auto_disarm_timeout = 30;
    config->safety_voltage_min = 6.5f;
    config->temperature_limit_max = 80.0f;
    config->motion_timeout_ms = 1000;
    config->startup_delay_ms = 2000;
    config->max_control_frequency = 100;

    uint8_t mac[6];
    esp_err_t err = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (err == ESP_OK) {
        snprintf(config->robot_id, sizeof(config->robot_id), "HEXAPOD_%02X%02X%02X", mac[3], mac[4], mac[5]);
    } else {
        strcpy(config->robot_id, "HEXAPOD_DEFAULT");
    }

    strcpy(config->robot_name, "My Hexapod Robot");
    config->config_version = CONFIG_SCHEMA_VERSION_DEFAULT;
    config->controller_type = CONTROLLER_DRIVER_FLYSKY_IBUS;

}

void config_load_joint_calib_defaults(joint_calib_config_t* config) {
    if (!config) {
        return;
    }

    memset(config, 0, sizeof(joint_calib_config_t));

    for (int leg = 0; leg < NUM_LEGS; leg++) {
        for (int joint = 0; joint < NUM_JOINTS_PER_LEG; joint++) {
            joint_calib_t* calib = &config->joints[leg][joint];
            config_domain_fill_joint_default(joint, calib);
        }
    }

}

void config_load_leg_geometry_defaults(leg_geometry_config_t* config) {
    if (!config) {
        return;
    }

    memset(config, 0, sizeof(leg_geometry_config_t));

    const float default_len_coxa = 0.068f;
    const float default_len_femur = 0.088f;
    const float default_len_tibia = 0.127f;

    const float x_off_front = 0.08f;
    const float x_off_rear = -0.08f;
    const float y_off_left = 0.05f;
    const float y_off_right = -0.05f;
    const float z_off = 0.0f;
    const float yaw_left = (float)M_PI * 0.5f;
    const float yaw_right = (float)-M_PI * 0.5f;
    const float qangle = (float)M_PI * 0.25f;

    const float default_stance_out = 0.15f;
    const float default_stance_fwd = 0.0f;

    for (int leg = 0; leg < NUM_LEGS; leg++) {
        config->len_coxa[leg] = default_len_coxa;
        config->len_femur[leg] = default_len_femur;
        config->len_tibia[leg] = default_len_tibia;

        config->mount_z[leg] = z_off;
        config->stance_out[leg] = default_stance_out;
        config->stance_fwd[leg] = default_stance_fwd;
    }

    // Left side
    config->mount_x[0] = x_off_front;
    config->mount_y[0] = y_off_left;
    config->mount_yaw[0] = yaw_left - qangle;

    config->mount_x[1] = 0.0f;
    config->mount_y[1] = y_off_left;
    config->mount_yaw[1] = yaw_left;

    config->mount_x[2] = x_off_rear;
    config->mount_y[2] = y_off_left;
    config->mount_yaw[2] = yaw_left + qangle;

    // Right side
    config->mount_x[3] = x_off_front;
    config->mount_y[3] = y_off_right;
    config->mount_yaw[3] = yaw_right + qangle;

    config->mount_x[4] = 0.0f;
    config->mount_y[4] = y_off_right;
    config->mount_yaw[4] = yaw_right;

    config->mount_x[5] = x_off_rear;
    config->mount_y[5] = y_off_right;
    config->mount_yaw[5] = yaw_right - qangle;
}
