#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware.h"

//Acciones a ejecutar según pedido desde navegador
void html_do_actions(char* request) {
    // Control LED
    if (strstr(request, "GET /led=on")) LED_ON();
    if (strstr(request, "GET /led=off")) LED_OFF();
}

//Generar página HTML
const char* html_page(int* len) {
    static char buffer[1024];
    int len_header;

    int content_len = snprintf(NULL, 0,
        "<!DOCTYPE html><html>"
        "<head><title>WEB Server</title></head>"
        "<body><h1>Servidor Web Pico 2W</h1>"
        "<p>LED (GPIO22) : %s</p>"
        "<a href=\"/led=on\"><button>LED On</button></a>"
        "<br><br>"
        "<a href=\"/led=off\"><button>LED Off</button></a>"
        "<br><br>"
        "<p>Pulsador (GPIO15) : %s</p>"
        "</body></html>",
        (LED_STATE() ? "Encendido" : "Apagado"),
        (PULS_STATE() ? "Alto" : "Bajo")
    );

    *len = content_len;

    snprintf(buffer, sizeof(buffer),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<!DOCTYPE html><html>"
        "<head><title>WEB Server</title></head>"
        "<body><h1>Servidor Web Pico 2W</h1>"
        "<p>LED (GPIO22) : %s</p>"
        "<a href=\"/led=on\"><button>LED On</button></a>"
        "<br><br>"
        "<a href=\"/led=off\"><button>LED Off</button></a>"
        "<br><br>"
        "<p>Pulsador (GPIO15) : %s</p>"
        "</body></html>",
        content_len,
        (LED_STATE() ? "Encendido" : "Apagado"),
        (PULS_STATE() ? "Alto" : "Bajo")
    );

    return buffer;
}
