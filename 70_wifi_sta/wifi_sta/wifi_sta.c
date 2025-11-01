#include <stdio.h>
#include "pico/stdlib.h"

#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"

#include "wifi_sta.h"
#include "wifi_sta_err.h"

//#define WIFI_STA_DEBUG              (1)

#define TOUT_REINTENTO_CONEXION_AP  (20000) //En mS, reintento de conexion al AP
#define TOUT_CHEQUEO_CONEXION_AP    (5000)  //En mS, chequeo de conexion al AP

enum {
    WIFI_OFFLINE,
    WIFI_INIT,
    WIFI_CONNECT,
    WIFI_CONNECTED,
};

volatile uint32_t tout_retry_wifi_sta = 0;
volatile uint32_t tout_check_connected = 0;
uint8_t cant_reintentos;

char* ssid_to_connect = NULL;
char* pass_to_connect = NULL;

uint8_t my_ip[4];

wifi_sta_data_t wifi_sta_data;

void set_ssid_to_connect(const char* ssid) {
    ssid_to_connect = (char*)(ssid);
}
void set_pass_to_connect(const char* pass) {
    pass_to_connect = (char*)(pass);
}

wifi_sta_data_t* ptr_wifi_data(void) {
    return (&wifi_sta_data);
}

/*
    Recibe en la estructura los datos de:
    SSID, PASS, AUTH y TOUT para conectarse a un AP
    Retorna WIFI_OK o error descritivo
    WIFI_ERR_TOUT - WIFI_ERR_BADAUTH - WIFI_ERR_CONNECTING
*/
wifi_sta_err_t wifi_connect(wifi_sta_data_t* wifi_data) {
    wifi_sta_err_t err = WIFI_STA_ERR_CONNECTING;
    int error;

    if (wifi_data->ssid) {
        cyw43_arch_enable_sta_mode();
        error = cyw43_arch_wifi_connect_timeout_ms(wifi_data->ssid, wifi_data->pass, wifi_data->auth, wifi_data->tout);
        if (error == PICO_OK) {
            wifi_data->my_ip = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
            err = WIFI_STA_OK;
        } else if (error == PICO_ERROR_TIMEOUT) {
            err = WIFI_STA_ERR_TOUT;
        } else if (error == PICO_ERROR_BADAUTH) {
            err = WIFI_STA_ERR_AUTH;
        }
    }
    return err;
}

void wifi_sta_fsm(void) {
    static uint8_t state = WIFI_OFFLINE;
    wifi_sta_err_t error;

    switch (state) {
    case WIFI_OFFLINE:
        //Debe verificar si tiene configurado SSID y PASS del AP
        if ((ssid_to_connect) && (pass_to_connect)) {
            state = WIFI_INIT;
        }
        break;
    case WIFI_INIT:
        wifi_sta_data.ssid = ssid_to_connect;
        wifi_sta_data.pass = pass_to_connect;
        wifi_sta_data.auth = CYW43_AUTH_WPA2_AES_PSK;
        wifi_sta_data.tout = 30000;
        tout_retry_wifi_sta = to_ms_since_boot(get_absolute_time());
        state = WIFI_CONNECT;
        break;
    case WIFI_CONNECT:
        if ((tout_retry_wifi_sta) && (tout_retry_wifi_sta <= to_ms_since_boot(get_absolute_time()))) {
            tout_retry_wifi_sta = 0;
#if (WIFI_STA_DEBUG == 1)
            printf("Conectando...\r\n");
#endif
            error = wifi_connect((wifi_sta_data_t*)(&wifi_sta_data));
            if (error == WIFI_STA_OK) {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                wifi_sta_data.wifi_status |= WIFI_CONECTED;
#if (WIFI_STA_DEBUG == 1)
                printf("Conectado al AP!!!\r\n");
                printf("Mi IP address %d.%d.%d.%d\r\n", wifi_sta_data.my_ip[0], wifi_sta_data.my_ip[1], wifi_sta_data.my_ip[2], wifi_sta_data.my_ip[3]);
#endif
                tout_check_connected = (to_ms_since_boot(get_absolute_time()) + TOUT_CHEQUEO_CONEXION_AP);
                state = WIFI_CONNECTED;
            } else {
                tout_retry_wifi_sta = to_ms_since_boot(get_absolute_time()) + TOUT_REINTENTO_CONEXION_AP;
#if (WIFI_STA_DEBUG == 1)
                if (error == WIFI_STA_ERR_TOUT) {
                    printf("Error timeout al conectar\r\n");
                } else if (error == WIFI_STA_ERR_AUTH) {
                    printf("Error autenticacion\r\n");
                } else {
                    printf("Error, verifique SSID y/o password\r\n");
                }
                printf("Reintento de conexion en %ds\r\n", (TOUT_REINTENTO_CONEXION_AP / 1000));
#endif
            }
        }
        break;
    case WIFI_CONNECTED:
        // Debe monitorear mientras está conectado si se cae
        if (tout_check_connected == to_ms_since_boot(get_absolute_time())) {
            tout_check_connected += TOUT_CHEQUEO_CONEXION_AP;
#if (WIFI_STA_DEBUG == 1)
            printf("Verifica la conexión al AP\r\n");
#endif
            if (cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) != CYW43_LINK_UP) {
                // Se cayó AP, reinicio conexión
                wifi_sta_data.wifi_status &= ~(WIFI_CONECTED);
#if (WIFI_STA_DEBUG == 1)
                printf("Se cayó la conexión al AP\r\n");
                printf("Reintento de conexion en %ds\r\n", (TOUT_REINTENTO_CONEXION_AP / 1000));
#endif
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                tout_retry_wifi_sta = to_ms_since_boot(get_absolute_time()) + TOUT_REINTENTO_CONEXION_AP;
                state = WIFI_CONNECT;
            }
        }
        break;
    }
}
