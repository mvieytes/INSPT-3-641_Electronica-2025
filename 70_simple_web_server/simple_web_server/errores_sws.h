#ifndef _ERRORES_SWX_H_
#define _ERRORES_SWX_H_

typedef enum {
    CYW43_INIT_FAIL = -128,
    BT_ERROR,
    WIFI_ERR_CONNECTING,
    WIFI_ERR_TOUT,
    WIFI_ERR_AUTH,
    TCP_NO_MEM,
    TCP_ERR_USE,
    CYW43_INIT_OK = 0,
    BT_OK = 0,
    WIFI_OK = 0,
    TCP_OK = 0,
    HTTP_OK = 0,
}error_t;

#endif