#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/cyw43_arch.h"
#include "web_server_err.h"
#include "web_server.h"
#include "http.h"

#define WEB_SERVER_DEBUG            (1)

web_server_data_t web_server_data;

enum {
    WEB_SERVER_UP,
    WEB_SEVER_ONLINE,
};

web_server_data_t* ptr_web_server_data(void) {
    return (&web_server_data);
}

void web_server_fsm(uint8_t wifi_sta_connected) {
    static uint8_t state = WEB_SERVER_UP;
    error_web_server_t error;

    switch (state) {
    case WEB_SERVER_UP:
        if (wifi_sta_connected) {
#if (WEB_SERVER_DEBUG == 1)
            printf("WEB SERVER: Levantando servidor...\r\n");
#endif
            error = init_http_server();
            if (error == TCP_OK) {
                web_server_data.web_server_status |= WEB_SERVER_ONLINE;
#if (WEB_SERVER_DEBUG == 1)
                printf("WEB SERVER: Servidor en linea!!!\r\n");
#endif
                state = WEB_SERVER_ONLINE;
            } else {
#if (WEB_SERVER_DEBUG == 1)
                if (error == TCP_NO_MEM) {
                    printf("WEB SERVER: Error sin memoria para crear puerto TCP\r\n");
                } else if (error == TCP_ERR_USE) {
                    printf("WEB SERVER: Error puerto TCP ocupado\r\n");
                }
                printf("WEB SERVER: Desconexión del AP\r\n");
#endif
                web_server_data.web_server_status &= ~(WEB_SERVER_ONLINE);
            }
        }
        break;
    case WEB_SERVER_ONLINE:
        // Debe monitorear si está online o se cae...
        if (wifi_sta_connected == 0) {
#if (WEB_SERVER_DEBUG == 1)
            printf("WEB SERVER: Deinit\r\n");
#endif
            web_server_data.web_server_status &= ~(WEB_SERVER_ONLINE);
            deinit_http_server();
            state = WEB_SERVER_UP;
        }
        break;
    }
}
