#ifndef _WEB_SERVER_H_
#define _WEB_SERVER_H_
#include "web_server_err.h"
#include "http.h"

#define WEB_SERVER_CONFIGURED     (1<<0)
#define WEB_SERVER_ONLINE         (1<<1)

typedef struct {
    uint8_t web_server_status;
}web_server_data_t;

web_server_data_t* ptr_web_server_data(void);
void web_server_fsm(uint8_t wifi_sta_connected);

#endif