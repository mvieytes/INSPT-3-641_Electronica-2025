#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware.h"

const char encabezado[] =
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html\r\n"
"Content-Length: %d\r\n"
"Connection: close\r\n"
"\r\n";

const char pagina[] =
"<!DOCTYPE html><html>"
"<head><title>INSPT Alarma</title></head>"
"<body><h1>Servidor Web Alarma INSPT Pico 2W</h1>"
"<p>LED Rojo (GPIO17) : %s</p>"
"<p>LED Verde (GPIO16) : %s</p>"
"<a href=\"/ledr=on\"><button>LEDR On</button></a>"
"<br><br>"
"<a href=\"/ledr=off\"><button>LEDR Off</button></a>"
"<br><br>"
"<a href=\"/ledv=on\"><button>LEDV On</button></a>"
"<br><br>"
"<a href=\"/ledv=off\"><button>LEDV Off</button></a>"
"<br><br>"
"<p>Pulsador 1 (GPIO12) : %s</p>"
"<p>Pulsador 2 (GPIO15) : %s</p>"
"</body></html>";

//Acciones a ejecutar según pedido desde navegador
void html_do_actions(char* request) {
    // Control LEDs
    if (strstr(request, "GET /ledr=on")) LEDR_ON();
    if (strstr(request, "GET /ledr=off")) LEDR_OFF();
    if (strstr(request, "GET /ledv=on")) LEDV_ON();
    if (strstr(request, "GET /ledv=off")) LEDV_OFF();
}

//Generar página HTML
const char* html_page(int* len) {
    static char buffer[1024];

    int content_len = snprintf(NULL, 0, pagina,
        (LEDR_STATE() ? "Encendido" : "Apagado"),
        (LEDV_STATE() ? "Encendido" : "Apagado"),
        (PULS1_STATE() ? "Alto" : "Bajo"),
        (PULS2_STATE() ? "Alto" : "Bajo")
    );

    *len = content_len;

    snprintf(buffer, sizeof(buffer), "%s%s", encabezado, pagina,
        content_len,
        (LEDR_STATE() ? "Encendido" : "Apagado"),
        (LEDV_STATE() ? "Encendido" : "Apagado"),
        (PULS1_STATE() ? "Alto" : "Bajo"),
        (PULS2_STATE() ? "Alto" : "Bajo")
    );

    return buffer;
}
