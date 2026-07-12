#include <Arduino.h>         /* millis() (core_pins.h, transitively) for eth_resolve's timeout */
#include "socket_lwip.h"
#include "lwip/priv/tcp_priv.h"
#include "ethernetif.h"     /* ethernetif_poll */
#include "lwip/timeouts.h"
#include "lwip/dns.h"

extern "C" { tcp_conn_t eth_conns[ETH_MAX_SOCK_NUM]; }

/* the single shared netif lives here; EthernetClass initializes it via eth_netif(). */
static struct netif s_netif;
struct netif *eth_netif(void) { return &s_netif; }

void eth_pump(void) {
    static bool in_pump = false;         /* reentrancy guard: write/connect spins call
                                            yield()->eth_pump(); never nest into lwIP. */
    if (in_pump) return;
    in_pump = true;
    ethernetif_poll(&s_netif);
    sys_check_timeouts();
    in_pump = false;
}

int eth_conn_alloc(void) {
    for (int i = 0; i < ETH_MAX_SOCK_NUM; i++)
        if (eth_conns[i].state == CONN_FREE) {
            eth_conns[i] = (tcp_conn_t){0};
            eth_conns[i].state = CONN_CONNECTING;   /* claimed; caller sets real state */
            return i;
        }
    return -1;
}

void eth_conn_free(int i) {
    if (i < 0) return;
    tcp_conn_t *c = &eth_conns[i];
    if (c->rx_head) { pbuf_free(c->rx_head); c->rx_head = nullptr; }
    if (c->pcb) {
        tcp_arg(c->pcb, nullptr);
        tcp_recv(c->pcb, nullptr); tcp_sent(c->pcb, nullptr); tcp_err(c->pcb, nullptr);
        if (tcp_close(c->pcb) != ERR_OK) tcp_abort(c->pcb);
        c->pcb = nullptr;
    }
    c->state = CONN_FREE;
}

err_t eth_recv_cb(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    int i = (int)(intptr_t)arg;
    tcp_conn_t *c = &eth_conns[i];
    if (p == nullptr) { c->state = CONN_CLOSING; return ERR_OK; }   /* peer FIN; keep rx_head */
    if (err != ERR_OK) { pbuf_free(p); return ERR_OK; }   /* p non-NULL here (FIN handled above) */
    if (c->rx_head) pbuf_cat(c->rx_head, p); else c->rx_head = p;   /* hold; recved on drain */
    return ERR_OK;
}

err_t eth_sent_cb(void *arg, struct tcp_pcb *pcb, uint16_t len) { (void)arg;(void)pcb;(void)len; return ERR_OK; }

void eth_err_cb(void *arg, err_t err) {
    int i = (int)(intptr_t)arg;                 /* lwIP has already freed the pcb */
    (void)err;
    if (i >= 0 && i < ETH_MAX_SOCK_NUM) { eth_conns[i].pcb = nullptr; eth_conns[i].state = CONN_CLOSED; }
}

void eth_conn_bind_callbacks(int i) {
    struct tcp_pcb *pcb = eth_conns[i].pcb;
    tcp_arg(pcb, (void *)(intptr_t)i);
    tcp_recv(pcb, eth_recv_cb);
    tcp_sent(pcb, eth_sent_cb);
    tcp_err(pcb, eth_err_cb);
}

int eth_conn_available(int i) {
    if (i < 0 || i >= ETH_MAX_SOCK_NUM) return 0;
    tcp_conn_t *c = &eth_conns[i];
    if (!c->rx_head) return 0;
    return (int)(c->rx_head->tot_len - c->rx_off);
}

int eth_conn_peek(int i) {
    if (i < 0 || i >= ETH_MAX_SOCK_NUM) return -1;
    tcp_conn_t *c = &eth_conns[i];
    if (!c->rx_head || c->rx_off >= c->rx_head->tot_len) return -1;
    return pbuf_get_at(c->rx_head, c->rx_off);
}

int eth_conn_read(int i, uint8_t *buf, int len) {
    if (i < 0 || i >= ETH_MAX_SOCK_NUM) return -1;
    tcp_conn_t *c = &eth_conns[i];
    if (!c->rx_head) return -1;
    int got = 0;
    while (got < len && c->rx_head) {
        uint16_t avail = c->rx_head->tot_len - c->rx_off;
        if (avail == 0) break;
        uint16_t n = (uint16_t)((len - got) < avail ? (len - got) : avail);
        pbuf_copy_partial(c->rx_head, buf + got, n, c->rx_off);
        got += n; c->rx_off += n;
        /* free whole pbufs off the head as they are consumed, crediting the window */
        while (c->rx_head && c->rx_off >= c->rx_head->len) {
            c->rx_off -= c->rx_head->len;
            uint16_t seg = c->rx_head->len;
            struct pbuf *next = c->rx_head->next;
            if (next) pbuf_ref(next);
            pbuf_free(c->rx_head);
            c->rx_head = next;
            if (c->pcb) tcp_recved(c->pcb, seg);
        }
    }
    return got ? got : -1;
}

struct dns_wait { volatile int done; volatile int ok; ip_addr_t addr; };
static void eth_dns_cb(const char *name, const ip_addr_t *ipaddr, void *arg) {
    (void)name; struct dns_wait *w = (struct dns_wait *)arg;
    if (ipaddr) { w->addr = *ipaddr; w->ok = 1; } else w->ok = 0;
    w->done = 1;
}
int eth_resolve(const char *host, ip_addr_t *out, uint32_t timeout_ms) {
    struct dns_wait w; w.done = 0; w.ok = 0;
    err_t e = dns_gethostbyname(host, &w.addr, eth_dns_cb, &w);
    if (e == ERR_OK) { *out = w.addr; return 1; }          /* cached */
    if (e != ERR_INPROGRESS) return 0;
    uint32_t t0 = millis();
    while (!w.done) { eth_pump(); if (millis() - t0 > timeout_ms) return 0; }
    if (w.ok) { *out = w.addr; return 1; }
    return 0;
}
