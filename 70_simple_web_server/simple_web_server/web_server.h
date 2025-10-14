#ifndef _WEB_SERVER_H_
#define _WEB_SERVER_H_

void set_ssid_to_connect(const char* ssid);
void set_pass_to_connect(const char* pass);

void web_server_init_data(void);
void web_server_fsm(void);

#endif