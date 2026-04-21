add_library(oled_i2c
    oled_i2c.h
    oled_i2c.c
    oled_fonts.h
    oled_graphics.h
    oled_graphics.c
)

target_link_libraries(oled_i2c
    pico_stdlib
    hardware_i2c
)

target_include_directories(oled_i2c PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")