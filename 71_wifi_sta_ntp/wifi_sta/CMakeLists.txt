add_library(wifi_sta
	wifi_sta_err.h
	wifi_sta.h
	wifi_sta.c
)

target_link_libraries(wifi_sta
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background)

target_include_directories(wifi_sta PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/..")