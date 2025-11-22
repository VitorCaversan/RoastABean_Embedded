idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES "DCMotor" "SPI" "Buttons" "OSConfig" "TriacControl" "PIDControl" "bt" "Bluetooth")
