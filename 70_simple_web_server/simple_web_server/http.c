#include <stdio.h>
#include "pico/stdlib.h"
#include "errores_sws.h"
#include "http.h"

#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"

#define HTTP_DEBUG

html_page_t html_page_to_serve;
html_actions_t html_actions;

struct tcp_pcb* pcb;

void set_html_to_serve(html_page_t html_page) {
    html_page_to_serve = html_page;
}
void set_html_actions(html_actions_t html_actions) {
    html_actions = html_actions;
}

/* Callback cuando se confirma que se envió todo */
error_t sent_callback(void* arg, struct tcp_pcb* tpcb, u16_t len) {
    // Limpiar callbacks antes de cerrar
    tcp_arg(tpcb, NULL);
    tcp_recv(tpcb, NULL);
    tcp_close(tpcb);
    return TCP_OK;
}
/* Función de recepción de peticiones HTTP entrantes */
error_t http_recv(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
    const char* response = NULL;

    if (!p) {
        tcp_close(tpcb);
        return HTTP_OK;
    }

    // Copiar solicitud
    char* req = malloc(p->tot_len + 1);
    pbuf_copy_partial(p, req, p->tot_len, 0);
    req[p->tot_len] = '\0';

#ifdef HTTP_DEBUG
    printf("Solicitud:\n%s\n", req);
#endif
    // Responde favicon.ico vacío
    if (strstr(req, "GET /favicon.ico")) {
        response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: image/x-icon\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n\r\n";
    }

    if (response == NULL) {
        // Generar respuesta HTML
        if (html_actions)
            html_actions(req);

        if (html_page_to_serve) {
            int content_len = 0;
            response = html_page_to_serve(&content_len);
#ifdef HTTP_DEBUG
            printf("Largo respuesta: %d\n", content_len);
#endif
        }
    }

#ifdef HTTP_DEBUG
    printf("Enviando respuesta:\n%s\n", response);
#endif
    // Enviar respuesta
    tcp_write(tpcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    tcp_sent(tpcb, sent_callback);
    free(req);
    pbuf_free(p);

    return HTTP_OK;
}
/* Registra la función que va a responder las peticiones entrantes */
static err_t http_accept(void* arg, struct tcp_pcb* newpcb, err_t err) {
    tcp_recv(newpcb, http_recv);
    return ERR_OK;
}
/*  Inicializa el servidor p/escuchar en el puerto 80
    Devuelve:
    - TCP_OK si pudo crear un puerto y queda escuchando
    - TCP_NO_MEM si no hay memoria para crear un puerto
    - TCP_ERR_USE si está siendo utilizado
*/
error_t init_http_server(void) {
    error_t error = TCP_OK;
    // Servidor TCP puerto 80
    // Crea un socket o pcb (Protocol Control Block)
    pcb = tcp_new();
    if (pcb) {
        // "tcp_bind" asocia ese socket a una IP y puerto 
        if (tcp_bind(pcb, IP_ADDR_ANY, 80) == ERR_OK) {
            // "tcp_listen" transforma al socket como escucha, destruye el anterior para ahorro de memoria
            pcb = tcp_listen(pcb);
            if (pcb) {
                // "tcp_accept" pone en escucha al socket y registra la función que atiende la conexión entrante
                tcp_accept(pcb, http_accept);
            } else
                error = TCP_NO_MEM;
        } else {
            tcp_close(pcb);
            error = TCP_ERR_USE;
        }

    } else
        error = TCP_NO_MEM;
    if ((pcb) && (error != TCP_OK)) {
        tcp_close(pcb);
    }
    return error;
}

void deinit_http_server(void) {
    tcp_close(pcb);
}
