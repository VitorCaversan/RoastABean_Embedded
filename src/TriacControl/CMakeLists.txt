idf_component_register(SRCS "TriacControl.c" "Esp32Dimmer/src/components/esp32-triac-dimmer-driver/esp32-triac-dimmer-driver.c"
                       INCLUDE_DIRS "." "Esp32Dimmer/src/components/esp32-triac-dimmer-driver/include"
                       REQUIRES "OSConfig" "driver")
