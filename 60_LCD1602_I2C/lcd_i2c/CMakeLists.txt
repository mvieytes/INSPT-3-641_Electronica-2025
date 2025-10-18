add_library(lcd_i2c
    lcd_i2c.h
    lcd_i2c.c
)

target_link_libraries(lcd_i2c
    pico_stdlib
    hardware_i2c
)

target_include_directories(lcd_i2c PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")