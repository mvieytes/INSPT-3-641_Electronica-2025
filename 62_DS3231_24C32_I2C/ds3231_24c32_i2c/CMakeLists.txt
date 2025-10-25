add_library(ds3231_24c32_i2c
	ds3231_24c32_i2c.h
	ds3231_24c32_i2c.c
	_24c32.h
	_24c32.c
	ds3231.h
	ds3231.c
)

target_link_libraries(ds3231_24c32_i2c
    pico_stdlib
    hardware_i2c
)

target_include_directories(ds3231_24c32_i2c PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")