idf_component_register(SRCS "SpiConfig.c" "TempSens.c"
                       INCLUDE_DIRS "."
                       REQUIRES "esp-idf-lib__max31855" "esp-idf-lib__esp_idf_lib_helpers" "driver")
