add_library(web_server
	web_server_err.h
	http.h
	http.c
    web_server.h
    web_server.c
)

target_link_libraries(web_server
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background)

target_include_directories(web_server PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/..")