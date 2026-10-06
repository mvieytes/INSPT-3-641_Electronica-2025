add_library(ssd1306_i2c
    ssd1306_i2c.h
    ssd1306_i2c.c
    ssd1306_fonts.h
    ssd1306_graphs.h
    ssd1306_graphs.c
)

target_link_libraries(ssd1306_i2c
    pico_stdlib
    hardware_i2c
)

target_include_directories(ssd1306_i2c PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")