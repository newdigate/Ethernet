#include "Ethernet.h"
#include "EthernetServer.h"
#include "EthernetClient.h"
#include "utility/socket_lwip.h"
#include "lwip/tcp.h"

static err_t eth_accept_cb(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if (err != ERR_OK || newpcb == nullptr) return ERR_VAL;
    uint16_t lport = (uint16_t)(intptr_t)arg;
    int i = eth_conn_alloc();
    if (i < 0) { tcp_abort(newpcb); return ERR_ABRT; }   /* pool full: refuse */
    tcp_conn_t *c = &eth_conns[i];
    c->pcb = newpcb; c->state = CONN_ESTABLISHED; c->accept_port = lport;
    c->remote_ip = newpcb->remote_ip; c->remote_port = newpcb->remote_port;
    eth_conn_bind_callbacks(i);
    return ERR_OK;
}

void EthernetServer::begin() {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) return;
    if (tcp_bind(pcb, IP_ANY_TYPE, _port) != ERR_OK) { tcp_close(pcb); return; }
    struct tcp_pcb *lp = tcp_listen(pcb);               /* frees pcb, returns listen pcb */
    if (!lp) { tcp_close(pcb); return; }
    _listen = lp;
    tcp_arg(lp, (void *)(intptr_t)_port);
    tcp_accept(lp, eth_accept_cb);
}

/* Reclaim finished server connections (peer FIN/RST, fully drained) so the
   4-slot pool does not leak under connection churn. A CONN_CLOSING/CLOSED slot
   with 0 unread bytes is done; free it (eth_conn_free tcp_closes a still-valid
   pcb, or just frees rx pbufs if err_cb already nulled the pcb). Slots with
   unread data are kept so connected()/read() still see them past a peer FIN. */
static void eth_server_reap(uint16_t port) {
    for (int i = 0; i < ETH_MAX_SOCK_NUM; i++) {
        tcp_conn_t *c = &eth_conns[i];
        if (c->accept_port == port &&
            (c->state == CONN_CLOSED || c->state == CONN_CLOSING) &&
            eth_conn_available(i) == 0)
            eth_conn_free(i);
    }
}

EthernetClient EthernetServer::available() {
    eth_pump();
    eth_server_reap(_port);
    for (int i = 0; i < ETH_MAX_SOCK_NUM; i++) {
        tcp_conn_t *c = &eth_conns[i];
        if (c->state != CONN_FREE && c->accept_port == _port && eth_conn_available(i) > 0)
            return EthernetClient(i);
    }
    return EthernetClient(-1);
}

EthernetClient EthernetServer::accept() {
    eth_pump();
    eth_server_reap(_port);
    for (int i = 0; i < ETH_MAX_SOCK_NUM; i++) {
        tcp_conn_t *c = &eth_conns[i];
        if (c->state == CONN_ESTABLISHED && c->accept_port == _port) {
            c->accept_port = 0;            /* hand off once (ownership transfer) */
            return EthernetClient(i);
        }
    }
    return EthernetClient(-1);
}

size_t EthernetServer::write(uint8_t b) { return write(&b, 1); }
size_t EthernetServer::write(const uint8_t *buf, size_t size) {
    size_t n = 0;
    for (int i = 0; i < ETH_MAX_SOCK_NUM; i++)
        if (eth_conns[i].state != CONN_FREE && eth_conns[i].accept_port == _port) {
            EthernetClient c(i); n = c.write(buf, size);
        }
    return n;
}
