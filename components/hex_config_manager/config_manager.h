/*
 * Configuration Manager
 * 
 * NVS-based configuration persistence with dual-method API for live tuning.
 * Supports memory-only updates for immediate robot response and persistent
 * updates for permanent storage.
 * 
 * License: Apache-2.0
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "config_manager_core_types.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Core Configuration Manager API
// =============================================================================

/**
 * @brief Initialize configuration manager
 * 
 * Opens NVS handles for all namespaces and loads system configuration.
 * Must be called before any other config operations.
 * 
 * @return ESP_OK on success, error code on failure
 */
esp_err_t config_manager_init(void);

/**
 * @brief Get current configuration manager state
 * 
 * @param[out] state Pointer to state structure to fill
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if state is NULL
 */
esp_err_t config_manager_get_state(config_manager_state_t *state);

/**
 * @brief Check if any namespace has unsaved changes
 * 
 * @return true if there are unsaved changes, false otherwise
 */
bool config_manager_has_dirty_data(void);

/**
 * @brief Save specific namespace to NVS
 * 
 * @param ns Namespace to save
 * @return ESP_OK on success, error code on failure
 */
esp_err_t config_manager_save_namespace(config_namespace_t ns);

/**
 * @brief Save specific namespace to NVS by namespace name
 *
 * @param namespace_str Namespace name (for example "system")
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if name is unknown, error code on failure
 */
esp_err_t config_manager_save_namespace_by_name(const char* namespace_str);

// =============================================================================
// Hybrid Parameter API (Approach B - Individual Parameters)
// =============================================================================

/**
 * @brief Get boolean parameter value
 * 
 * @param namespace_str Namespace name (e.g., "system")
 * @param param_name Parameter name (e.g., "emergency_stop_enabled")
 * @param[out] value Pointer to store the value
 * @return ESP_OK on success, error code on failure
 */
esp_err_t hexapod_config_get_bool(const char* namespace_str, const char* param_name, bool* value);

/**
 * @brief Set boolean parameter value
 * 
 * @param namespace_str Namespace name
 * @param param_name Parameter name
 * @param value New value
 * @param persist true to save to NVS, false for memory-only
 * @return ESP_OK on success, error code on failure
 */
esp_err_t hexapod_config_set_bool(const char* namespace_str, const char* param_name, bool value, bool persist);

/**
 * @brief Get 32-bit integer parameter value
 * 
 * @param namespace_str Namespace name
 * @param param_name Parameter name
 * @param[out] value Pointer to store the value
 * @return ESP_OK on success, error code on failure
 */
esp_err_t hexapod_config_get_int32(const char* namespace_str, const char* param_name, int32_t* value);

/**
 * @brief Set 32-bit integer parameter value
 * 
 * @param namespace_str Namespace name
 * @param param_name Parameter name
 * @param value New value
 * @param persist true to save to NVS, false for memory-only
 * @return ESP_OK on success, error code on failure
 */
esp_err_t hexapod_config_set_int32(const char* namespace_str, const char* param_name, int32_t value, bool persist);

/**
 * @brief Get unsigned 32-bit integer parameter value
 * 
 * @param namespace_str Namespace name
 * @param param_name Parameter name
 * @param[out] value Pointer to store the value
 * @return ESP_OK on success, error code on failure
 */
esp_err_t hexapod_config_get_uint32(const char* namespace_str, const char* param_name, uint32_t* value);

/**
 * @brief Set unsigned 32-bit integer parameter value
 * 
 * @param namespace_str Namespace name
 * @param param_name Parameter name
 * @param value New value
 * @param persist true to save to NVS, false for memory-only
 * @return ESP_OK on success, error code on failure
 */
esp_err_t hexapod_config_set_uint32(const char* namespace_str, const char* param_name, uint32_t value, bool persist);

/**
 * @brief Get float parameter value
 * 
 * @param namespace_str Namespace name
 * @param param_name Parameter name
 * @param[out] value Pointer to store the value
 * @return ESP_OK on success, error code on failure
 */
esp_err_t hexapod_config_get_float(const char* namespace_str, const char* param_name, float* value);

/**
 * @brief Set float parameter value
 * 
 * @param namespace_str Namespace name
 * @param param_name Parameter name
 * @param value New value
 * @param persist true to save to NVS, false for memory-only
 * @return ESP_OK on success, error code on failure
 */
esp_err_t hexapod_config_set_float(const char* namespace_str, const char* param_name, float value, bool persist);

/**
 * @brief Get string parameter value
 * 
 * @param namespace_str Namespace name
 * @param param_name Parameter name
 * @param[out] value Buffer to store the string
 * @param max_len Maximum length of the buffer (including null terminator)
 * @return ESP_OK on success, error code on failure
 */
esp_err_t hexapod_config_get_string(const char* namespace_str, const char* param_name, char* value, size_t max_len);

/**
 * @brief Set string parameter value
 * 
 * @param namespace_str Namespace name
 * @param param_name Parameter name
 * @param value New string value
 * @param persist true to save to NVS, false for memory-only
 * @return ESP_OK on success, error code on failure
 */
esp_err_t hexapod_config_set_string(const char* namespace_str, const char* param_name, const char* value, bool persist);

// =============================================================================
// Parameter Discovery and Metadata API
// =============================================================================

/**
 * @brief List all available namespaces
 * 
 * @param[out] namespace_names Array to store namespace name pointers
 * @param[out] count Number of namespaces returned
 * @return ESP_OK on success, error code on failure
 */
esp_err_t config_list_namespaces(const char** namespace_names, size_t* count);

/**
 * @brief List all parameters in a namespace
 * 
 * @param namespace_str Namespace name
 * @param[out] param_names Array to store parameter name pointers
 * @param max_params Maximum number of parameters to return
 * @param[out] count Number of parameters returned
 * @return ESP_OK on success, error code on failure
 */
esp_err_t config_list_parameters(const char* namespace_str, const char** param_names, 
                                size_t max_params, size_t* count);

/**
 * @brief Get parameter metadata and constraints
 * 
 * @param namespace_str Namespace name
 * @param param_name Parameter name
 * @param[out] info Parameter information structure to fill
 * @return ESP_OK on success, error code on failure
 */
esp_err_t config_get_parameter_info(const char* namespace_str, const char* param_name, 
                                   config_param_info_t* info);

// =============================================================================
// Configuration Defaults and Factory Reset
// =============================================================================

/**
 * @brief Factory reset - restore all configuration to defaults
 * 
 * @return ESP_OK on success, error code on failure
 */
esp_err_t config_factory_reset(void);

// =============================================================================
// WiFi Partition Utilities (Optional)
// =============================================================================

/**
 * Read WiFi credentials from the default ESP-IDF WiFi partition
 * This is separate from robot configuration storage
 * 
 * @param ssid Buffer for WiFi SSID
 * @param ssid_len Size of SSID buffer  
 * @param password Buffer for WiFi password
 * @param password_len Size of password buffer
 * @return ESP_OK on success, ESP_ERR_NVS_NOT_FOUND if no credentials stored
 */
esp_err_t config_get_wifi_credentials(char* ssid, size_t ssid_len, char* password, size_t password_len);

#ifdef __cplusplus
}
#endif