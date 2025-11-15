idf_component_register(SRCS "Display.c" "SpiConfig.c" "TempSens.c"
                       INCLUDE_DIRS "."
                       REQUIRES "esp-idf-lib__max31855" "esp-idf-lib__esp_idf_lib_helpers" "driver" "lvgl__lvgl" "espressif__esp_lvgl_port" "espressif__esp_lcd_ili9341" "Buttons")
