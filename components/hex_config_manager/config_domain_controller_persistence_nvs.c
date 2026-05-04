#include "config_domain_persistence_nvs.h"

#include "config_domain_defaults.h"
#include "esp_log.h"

static const char *TAG = "cfg_ctrl_nvs";

#define CTRL_KEY_DRIVER_TYPE     "drv_t"
#define CTRL_KEY_TASK_STACK      "tsk_stk"
#define CTRL_KEY_TASK_PRIORITY   "tsk_pri"
#define CTRL_KEY_FLYSKY_UART     "fly_u"
#define CTRL_KEY_FLYSKY_TX       "fly_tx"
#define CTRL_KEY_FLYSKY_RX       "fly_rx"
#define CTRL_KEY_FLYSKY_RTS      "fly_rts"
#define CTRL_KEY_FLYSKY_CTS      "fly_cts"
#define CTRL_KEY_FLYSKY_BAUD     "fly_baud"

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
