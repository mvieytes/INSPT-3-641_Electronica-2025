#ifndef _WIFI_STA_H_
#define _WIFI_STA_H_
#include "wifi_sta_err.h"

#define WIFI_CONFIGURED     (1<<0)
#define WIFI_CONECTED       (1<<1)

typedef struct {
    char* ssid;
    char* pass;
    uint32_t auth;
    uint32_t tout;
    uint8_t* my_ip;
    uint8_t wifi_status;
}wifi_sta_data_t;

void set_ssid_to_connect(const char* ssid);
void set_pass_to_connect(const char* pass);

wifi_sta_data_t* ptr_wifi_data(void);
void wifi_sta_fsm(void);

#endif