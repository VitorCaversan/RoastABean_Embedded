idf_component_register(SRCS "tb6612.c" "DCMotor.c"
                       INCLUDE_DIRS "."
                       REQUIRES "espressif__bdc_motor" "espressif__pid_ctrl" "driver" "esp_timer" "OSConfig")
