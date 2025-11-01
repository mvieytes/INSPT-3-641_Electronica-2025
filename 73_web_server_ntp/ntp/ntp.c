#include <string.h>
#include <time.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

#include "ntp.h"

//#define NTP_DEBUG               (1)

#define TOUT_REINTENTO_NTP      (10000) //En mS, reintento de conexión NTP

#define NTP_SERVER              "pool.ntp.org"
#define NTP_MSG_LEN             (48)
#define NTP_PORT                (123)
#define NTP_DELTA               (2208988800)    // seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_GMT_ADJUST          (10800)         //3hs en segundos (3*60*60)
#define NTP_TEST_TIME_MS        (60 * 1000)     //1min en segundos pasado a mS p/próxima actualización
#define NTP_RESEND_TIME_MS      (10 * 1000)

enum {
    NTP_OFFLINE,
    NTP_START,
    NTP_IDLE,
};

volatile uint32_t tout_retry_ntp = 0;

static ntp_result_t actual_ntp;

typedef struct NTP_T_ {
    ip_addr_t ntp_server_address;
    struct udp_pcb* ntp_pcb;
    async_at_time_worker_t request_worker;
    async_at_time_worker_t resend_worker;
} NTP_T;
NTP_T* state_ntp;

ntp_result_t* ptr_ntp_actual(void) {
    return &actual_ntp;
}

// Called with results of operation
static void ntp_result(NTP_T* state, int status, time_t* result) {
    if (status == 0 && result) {
        actual_ntp.epoch = *result;
        actual_ntp.ajutime = gmtime(result);
        actual_ntp.new_time = true;

        struct tm* utc = gmtime(result);
#ifdef NTP_DEBUG
        printf("Respuesta NTP: %02d/%02d/%04d %02d:%02d:%02d\n", utc->tm_mday, utc->tm_mon + 1, utc->tm_year + 1900,
            utc->tm_hour, utc->tm_min, utc->tm_sec);
#endif
    }
    async_context_remove_at_time_worker(cyw43_arch_async_context(), &state->resend_worker);
    hard_assert(async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &state->request_worker, NTP_TEST_TIME_MS)); // repeat the request in future
#ifdef NTP_DEBUG
    printf("Proxima conexion NTP en %ds\n", NTP_TEST_TIME_MS / 1000);
#endif
}

// Make an NTP request
static void ntp_request(NTP_T* state) {
    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();
    struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    uint8_t* req = (uint8_t*)p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;
    udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
    pbuf_free(p);
    cyw43_arch_lwip_end();
}

// Call back with a DNS result
static void ntp_dns_found(const char* hostname, const ip_addr_t* ipaddr, void* arg) {
    NTP_T* state = (NTP_T*)arg;
    if (ipaddr) {
        state->ntp_server_address = *ipaddr;
#ifdef NTP_DEBUG
        printf("NTP address %s\n", ipaddr_ntoa(ipaddr));
#endif
        ntp_request(state);
    } else {
#ifdef NTP_DEBUG
        printf("Falla requerimiento DNS NTP\n");
#endif
        ntp_result(state, -1, NULL);
    }
}

// NTP data received
static void ntp_recv(void* arg, struct udp_pcb* pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port) {
    NTP_T* state = (NTP_T*)arg;
    uint8_t mode = pbuf_get_at(p, 0) & 0x7;
    uint8_t stratum = pbuf_get_at(p, 1);

    // Check the result
    if ((ip_addr_cmp(addr, &state->ntp_server_address)) && (port == NTP_PORT) && (p->tot_len == NTP_MSG_LEN) && (mode == 0x4) && (stratum != 0)) {
        uint8_t seconds_buf[4] = { 0 };
        pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
        uint32_t seconds_since_1900 = seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];
        uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
        time_t epoch = seconds_since_1970 - NTP_GMT_ADJUST;
        ntp_result(state, 0, &epoch);
    } else {
#ifdef NTP_DEBUG
        printf("Respuesta NTP invalida\n");
#endif
        ntp_result(state, -1, NULL);
    }
    pbuf_free(p);
}

// Called to make a NTP request
static void request_worker_fn(__unused async_context_t* context, async_at_time_worker_t* worker) {
    NTP_T* state = (NTP_T*)worker->user_data;
    hard_assert(async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &state->resend_worker, NTP_RESEND_TIME_MS)); // in case UDP request is lost
    int err = dns_gethostbyname(NTP_SERVER, &state->ntp_server_address, ntp_dns_found, state);
    if (err == ERR_OK) {
        ntp_request(state); // Cached DNS result, make NTP request
    } else if (err != ERR_INPROGRESS) { // ERR_INPROGRESS means expect a callback
#ifdef NTP_DEBUG
        printf("Fallo requerimiento DNS\n");
#endif
        ntp_result(state, -1, NULL);
    }
}

// Called to resend an NTP request if it appears to get lost
static void resend_worker_fn(__unused async_context_t* context, async_at_time_worker_t* worker) {
    NTP_T* state = (NTP_T*)worker->user_data;
#ifdef NTP_DEBUG
    printf("Fallo requerimiento NTP\n");
#endif
    ntp_result(state, -1, NULL);
}

// Perform initialisation
void ntp_init(void) {
    state_ntp = malloc(sizeof(NTP_T));
    if (state_ntp != NULL) {
        state_ntp->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
        if (state_ntp->ntp_pcb == NULL) {
#ifdef NTP_DEBUG
            printf("Falla al crear PCB\n");
#endif
            free(state_ntp);
            state_ntp = NULL;
        }
        udp_recv(state_ntp->ntp_pcb, ntp_recv, state_ntp);
        state_ntp->request_worker.do_work = request_worker_fn;
        state_ntp->request_worker.user_data = state_ntp;
        state_ntp->resend_worker.do_work = resend_worker_fn;
        state_ntp->resend_worker.user_data = state_ntp;
    } else {
#ifdef NTP_DEBUG
        printf("Falla asignacion de memoria a state_ntp\n");
#endif
        state_ntp = NULL;
    }
}

void ntp_fsm(uint8_t wifi_sta_connected) {
    static uint8_t state = NTP_OFFLINE;

    switch (state) {

    case NTP_OFFLINE:
        // Espera que estemos conectados a un AP
        if (wifi_sta_connected) {
            tout_retry_ntp = to_ms_since_boot(get_absolute_time()) + TOUT_REINTENTO_NTP;
            state = NTP_START;
        }
        break;
    case NTP_START:
        if ((wifi_sta_connected) && (tout_retry_ntp <= to_ms_since_boot(get_absolute_time()))) {
            ntp_init();
            if (state_ntp) {
                hard_assert(async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &state_ntp->request_worker, 0));
                state = NTP_IDLE;
            } else {
                tout_retry_ntp = to_ms_since_boot(get_absolute_time()) + TOUT_REINTENTO_NTP;
            }
        }
        break;
    case NTP_IDLE:
        if (wifi_sta_connected == 0) {
            free(state_ntp);
            state = NTP_OFFLINE;
        }
    }
}
