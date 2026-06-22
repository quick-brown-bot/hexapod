idf_component_register(
    SRCS "robot_control.c"
    INCLUDE_DIRS "."
    REQUIRES
    driver
    hex_robot_config
    hex_kinematics
    hex_locomotion
)
