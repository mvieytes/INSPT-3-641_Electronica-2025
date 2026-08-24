#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

enum {
    STATE_RX_START,
    STATE_RX_PRIMER,
    STATE_RX_SECOND,
    STATE_RX_END,
};

bool mef_recepcion(uint8_t dato) {
    bool rta = false;
    static uint8_t estado = STATE_RX_START;

    switch (estado) {
    case STATE_RX_START:
        if (dato == 'H')
            estado = STATE_RX_PRIMER;
        break;
    case STATE_RX_PRIMER:
        if (dato == 'O')
            estado = STATE_RX_SECOND;
        else
            estado = STATE_RX_START;
        break;
    case STATE_RX_SECOND:
        if (dato == 'L')
            estado = STATE_RX_END;
        else
            estado = STATE_RX_START;
        break;
    case STATE_RX_END:
        if (dato == 'A')
            rta = true;
        estado = STATE_RX_START;
        break;
    default:
        break;
    }
    return rta;
}