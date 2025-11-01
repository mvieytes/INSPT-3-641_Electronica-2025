#ifndef _ERRORES_WEB_SERVER_H_
#define _ERRORES_WEB_SERVER_H_

typedef enum {
    TCP_NO_MEM = -128,
    TCP_ERR_USE,
    TCP_OK = 0,
    HTTP_OK = 0,
}error_web_server_t;

#endif