/*
 * Domain defaults for configuration namespaces.
 *
 * Centralizes in-memory default population for system and joint calibration
 * domains and exposes a reusable joint-default helper.
 */

#pragma once

#include "config_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

void config_domain_fill_joint_default(int joint, joint_calib_t* calib);

void config_load_system_defaults(system_config_t* config);
void config_load_joint_calib_defaults(joint_calib_config_t* config);

#ifdef __cplusplus
}
#endif
