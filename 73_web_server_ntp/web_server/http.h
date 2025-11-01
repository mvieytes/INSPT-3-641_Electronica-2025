#ifndef _HTTP_H_
#define _HTTP_H_

typedef const char* (*html_page_t)(int* len);
typedef void (*html_actions_t)(char* request);

void set_html_to_serve(html_page_t html_page);
void set_html_actions(html_actions_t html_actions);

error_web_server_t init_http_server(void);
void deinit_http_server(void);

#endif