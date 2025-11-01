#ifndef _NTP_H_
#define _NTP_H_
#include <time.h>

typedef struct {
    bool new_time;
    time_t epoch;
    struct tm* ajutime;
}ntp_result_t;

ntp_result_t* ptr_ntp_actual(void);
void ntp_fsm(uint8_t wifi_sta_connected);

#endif