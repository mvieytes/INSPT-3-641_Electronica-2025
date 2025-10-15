#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/cyw43_arch.h"
#include "debug_sws_config.h"
#include "web_server.h"
#include "wifi.h"
#include "http.h"

#define TOUT_REINTENTO_CONEXION_AP      (20000) //En mS, reintento de conexion al AP
#define TOUT_CHEQUEO_CONEXION_AP        (5000)  //En mS, chequeo de conexion al AP

volatile uint32_t tout_retry_web_server = 0;
volatile uint32_t tout_check_connected = 0;
uint8_t cant_reintentos;

char* ssid_to_connect = NULL;
char* pass_to_connect = NULL;

uint8_t my_ip[4];

wifi_sta_data_t* web_server_wifi_data;

enum {
    WIFI_OFFLINE,
    WIFI_INIT,
    WIFI_CONNECT,
    WIFI_SERVER_UP,
    WIFI_ONLINE,
};

void set_ssid_to_connect(const char* ssid) {
    ssid_to_connect = (char *)(ssid);
}
void set_pass_to_connect(const char* pass) {
    pass_to_connect = (char *)(pass);
}

void web_server_init_data(void) {
    web_server_wifi_data = ptr_wifi_data();
}

void web_server_fsm(void) {
    static uint8_t state = WIFI_OFFLINE;
    error_t error;

    switch (state) {
    case WIFI_OFFLINE:
        //Debe verificar si tiene configurado SSID y PASS del AP
        if ((ssid_to_connect) && (pass_to_connect)) {
            web_server_wifi_data->wifi_status |= WIFI_CONFIGURED;
            state = WIFI_INIT;
        }
        break;
    case WIFI_INIT:
        web_server_wifi_data->ssid = ssid_to_connect;
        web_server_wifi_data->pass = pass_to_connect;
        web_server_wifi_data->auth = CYW43_AUTH_WPA2_AES_PSK;
        web_server_wifi_data->tout = 30000;
        tout_retry_web_server = to_ms_since_boot(get_absolute_time());
        state = WIFI_CONNECT;
        break;
    case WIFI_CONNECT:
        if ((tout_retry_web_server) && (tout_retry_web_server <= to_ms_since_boot(get_absolute_time()))) {
            tout_retry_web_server = 0;
#if (WEB_SERVER_DEBUG == 1)
            printf("Conectando...\r\n");
#endif
            error = wifi_connect(web_server_wifi_data);
            if (error == WIFI_OK) {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                web_server_wifi_data->wifi_status |= WIFI_CONECTED;
#if (WEB_SERVER_DEBUG == 1)
                printf("Conectado al AP!!!\r\n");
                printf("Mi IP address %d.%d.%d.%d\r\n", web_server_wifi_data->my_ip[0], web_server_wifi_data->my_ip[1], web_server_wifi_data->my_ip[2], web_server_wifi_data->my_ip[3]);
#endif
                state = WIFI_SERVER_UP;
            } else {
                tout_retry_web_server = to_ms_since_boot(get_absolute_time()) + TOUT_REINTENTO_CONEXION_AP;
#if (WEB_SERVER_DEBUG == 1)
                if (error == WIFI_ERR_TOUT) {
                    printf("Error timeout al conectar\r\n");
                } else if (error == WIFI_ERR_AUTH) {
                    printf("Error autenticacion\r\n");
                } else {
                    printf("Error, verifique SSID y/o password\r\n");
                }
#endif
            }
        }
        break;
    case WIFI_SERVER_UP:
#if (WEB_SERVER_DEBUG == 1)
        printf("Levantando servidor...\r\n");
#endif
        error = init_http_server();
        if (error == TCP_OK) {
            web_server_wifi_data->wifi_status |= WIFI_SERVER_ONLINE;
#if (WEB_SERVER_DEBUG == 1)
            printf("Servidor en linea!!!\r\n");
#endif
            tout_check_connected = to_ms_since_boot(get_absolute_time()) + TOUT_CHEQUEO_CONEXION_AP;
            state = WIFI_ONLINE;
        } else {
#if (WEB_SERVER_DEBUG == 1)
            if (error == TCP_NO_MEM) {
                printf("Error sin memoria para crear puerto TCP\r\n");
            } else if (error == TCP_ERR_USE) {
                printf("Error puerto TCP ocupado\r\n");
            }
            printf("Desconexión del AP\r\n");
#endif
            web_server_wifi_data->wifi_status &= ~(WIFI_CONECTED | WIFI_SERVER_ONLINE);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
            tout_retry_web_server = to_ms_since_boot(get_absolute_time()) + TOUT_REINTENTO_CONEXION_AP;
            state = WIFI_CONNECT;
        }
        break;
    case WIFI_ONLINE:
        // Debe monitorear si está online o se cae...
        if (tout_check_connected == to_ms_since_boot(get_absolute_time())) {
            tout_check_connected += TOUT_CHEQUEO_CONEXION_AP;
            if (cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) != CYW43_LINK_UP) {
                // Se cayó AP, reinicio conexión
                web_server_wifi_data->wifi_status &= ~(WIFI_CONECTED | WIFI_SERVER_ONLINE);
#if (WEB_SERVER_DEBUG == 1)
                printf("Se cayó la conexión al AP\r\n");
#endif
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                deinit_http_server();
                tout_retry_web_server = to_ms_since_boot(get_absolute_time()) + TOUT_REINTENTO_CONEXION_AP;
                state = WIFI_CONNECT;
            }
        }
        break;
    }
}
