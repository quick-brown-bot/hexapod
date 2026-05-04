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

void config_load_motion_limits_defaults(motion_limits_config_t* config) {
    if (!config) {
        return;
    }

    memset(config, 0, sizeof(motion_limits_config_t));

    // Keep these aligned with current runtime tuning defaults in kpp_config.h.
    config->max_velocity[0] = 5.0f;
    config->max_velocity[1] = 5.0f;
    config->max_velocity[2] = 5.0f;

    config->max_acceleration[0] = 600.0f;
    config->max_acceleration[1] = 600.0f;
    config->max_acceleration[2] = 600.0f;

    config->max_jerk[0] = 3500.0f;
    config->max_jerk[1] = 3500.0f;
    config->max_jerk[2] = 3500.0f;

    config->velocity_filter_alpha = 0.3f;
    config->accel_filter_alpha = 0.2f;
    config->leg_velocity_filter_alpha = 0.25f;
    config->body_velocity_filter_alpha = 0.2f;
    config->body_pitch_filter_alpha = 0.1f;
    config->body_roll_filter_alpha = 0.1f;

    config->front_to_back_distance = 0.15f;
    config->left_to_right_distance = 0.12f;
    config->max_leg_velocity = 3.0f;
    config->max_body_velocity = 1.0f;
    config->max_angular_velocity = 5.0f;
    config->min_dt = 0.001f;
    config->max_dt = 0.050f;

    config->body_offset_x = 0.0f;
    config->body_offset_y = 0.0f;
    config->body_offset_z = 0.0f;
}

void config_load_controller_defaults(controller_config_namespace_t* config) {
    if (!config) {
        return;
    }

    memset(config, 0, sizeof(controller_config_namespace_t));

    config->driver_type = CONTROLLER_DRIVER_FLYSKY_IBUS;
    config->task_stack = 4096;
    config->task_priority = 10;

    config->flysky_uart_port = 1;
    config->flysky_tx_gpio = -1;
    config->flysky_rx_gpio = 22;
    config->flysky_rts_gpio = -1;
    config->flysky_cts_gpio = -1;
    config->flysky_baud_rate = 115200;
}

void config_load_wifi_defaults(wifi_config_namespace_t* config) {
    if (!config) {
        return;
    }

    memset(config, 0, sizeof(wifi_config_namespace_t));

    config->ap_ssid_mode = 1; // WIFI_AP_SSID_MAC_SUFFIX in wifi_ap.h
    strcpy(config->ap_fixed_prefix, "HEXAPOD");
    strcpy(config->ap_fixed_ssid, "HEXAPOD_AP");
    strcpy(config->ap_password, "HEXAPOD_ESP32");
    config->ap_channel = 1;
    config->ap_max_clients = 4;
    config->tcp_listen_port = 5555;
    config->tcp_connection_timeout_ms = 60000;
}
