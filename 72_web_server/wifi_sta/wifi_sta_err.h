#ifndef _WIFI_STA_ERR_H_
#define _WIFI_STA_ERR_H_

typedef enum {
    WIFI_STA_ERR_CONNECTING = -128,
    WIFI_STA_ERR_TOUT,
    WIFI_STA_ERR_AUTH,
    WIFI_STA_OK = 0,
}wifi_sta_err_t;

#endif