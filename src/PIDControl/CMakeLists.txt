idf_component_register(SRCS "PIDControl.c"
                       INCLUDE_DIRS "."
                       REQUIRES "OSConfig" "driver" "espressif__pid_ctrl" "esp_timer" "TriacControl" "SPI")
