#include <stdio.h>
#include "pico/stdlib.h"

#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"

#include "wifi.h"
#include "errores_sws.h"

wifi_sta_data_t wifi_sta_data;


wifi_sta_data_t* ptr_wifi_data(void) {
    return (&wifi_sta_data);
}

/*
    Recibe en la estructura los datos de:
    SSID, PASS, AUTH y TOUT para conectarse a un AP
    Retorna WIFI_OK o error descritivo
    WIFI_ERR_TOUT - WIFI_ERR_BADAUTH - WIFI_ERR_CONNECTING
*/
error_t wifi_connect(wifi_sta_data_t* wifi_data) {
    error_t err = WIFI_ERR_CONNECTING;
    int error;

    if (wifi_data->ssid) {
        cyw43_arch_enable_sta_mode();
        error = cyw43_arch_wifi_connect_timeout_ms(wifi_data->ssid, wifi_data->pass, wifi_data->auth, wifi_data->tout);
        if (error == PICO_OK) {
            wifi_data->my_ip = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
            err = WIFI_OK;
        } else if (error == PICO_ERROR_TIMEOUT) {
            err = WIFI_ERR_TOUT;
        } else if (error == PICO_ERROR_BADAUTH) {
            err = WIFI_ERR_AUTH;
        }
    }
    return err;
}
