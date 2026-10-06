add_library(ntp
	ntp.h
	ntp.c
)

target_link_libraries(ntp
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background)

target_include_directories(ntp PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/..")