idf_component_register(SRCS "BtnHndlrs.c" "Buttons.c"
                       INCLUDE_DIRS "."
                       REQUIRES "OSConfig" "driver" "esp_timer" "SPI" "PIDControl" "DCMotor" "Bluetooth" "NVShndlr" "Utils")
