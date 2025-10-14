add_library(simple_web_server
	simple_ws.h
	errores_sws.h
	http.h
	http.c
    web_server.h
    web_server.c
	wifi.h
	wifi.c
)

target_link_libraries(simple_web_server
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background)

target_include_directories(simple_web_server PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")