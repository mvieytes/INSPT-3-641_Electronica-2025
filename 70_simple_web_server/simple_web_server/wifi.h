#ifndef _WIFI_H_
#define _WIFI_H_
#include "errores_sws.h"

#define WIFI_CONFIGURED     (1<<0)
#define WIFI_CONECTED       (1<<1)
#define WIFI_SERVER_ONLINE  (1<<2)

typedef struct {
    char* ssid;
    char* pass;
    uint32_t auth;
    uint32_t tout;
    uint8_t* my_ip;
    uint8_t wifi_status;
}wifi_sta_data_t;

wifi_sta_data_t* ptr_wifi_data(void);
error_t wifi_connect(wifi_sta_data_t* wifi_data);

#endif