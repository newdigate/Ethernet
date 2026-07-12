#include "Ethernet.h"
#include "EthernetClient.h"
#include "utility/socket_lwip.h"
#include "lwip/tcp.h"

EthernetClient::EthernetClient() : _sock(-1), _gen(0) {}
EthernetClient::EthernetClient(int s) : _sock(s), _gen((s >= 0 && s < ETH_MAX_SOCK_NUM) ? eth_conns[s].gen : 0) {}
bool EthernetClient::_valid() const {
    return _sock >= 0 && _sock < ETH_MAX_SOCK_NUM &&
           eth_conns[_sock].gen == _gen && eth_conns[_sock].state != CONN_FREE;
}

static err_t eth_connected_cb(void *arg, struct tcp_pcb *pcb, err_t err) {
    (void)pcb; int i = (int)(intptr_t)arg;
    if (err == ERR_OK) eth_conns[i].state = CONN_ESTABLISHED;
    return ERR_OK;
}

int EthernetClient::connect(IPAddress ip, uint16_t port) {
    int i = eth_conn_alloc();
    if (i < 0) return 0;
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) { eth_conns[i].state = CONN_FREE; return 0; }
    eth_conns[i].pcb = pcb; eth_conns[i].state = CONN_CONNECTING;
    eth_conn_bind_callbacks(i);
    ip_addr_t dst; IP_ADDR4(&dst, ip[0], ip[1], ip[2], ip[3]);
    if (tcp_connect(pcb, &dst, port, eth_connected_cb) != ERR_OK) { eth_conn_free(i); return 0; }
    _sock = i; _gen = eth_conns[i].gen;
    uint32_t t0 = millis();
    while (eth_conns[i].state == CONN_CONNECTING) {
        eth_pump();
        if (millis() - t0 > 5000) { eth_conn_free(i); _sock = -1; return 0; }
    }
    if (eth_conns[i].state != CONN_ESTABLISHED) { eth_conn_free(i); _sock = -1; return 0; }
    return 1;
}

int EthernetClient::connect(const char *host, uint16_t port) {
    ip_addr_t a;
    if (!eth_resolve(host, &a, 8000)) return 0;
    uint32_t v = ip_2_ip4(&a)->addr;
    return connect(IPAddress(v&0xff,(v>>8)&0xff,(v>>16)&0xff,(v>>24)&0xff), port);
}

size_t EthernetClient::write(uint8_t b) { return write(&b, 1); }

size_t EthernetClient::write(const uint8_t *buf, size_t size) {
    if (!_valid()) return 0;
    tcp_conn_t *c = &eth_conns[_sock];
    size_t sent = 0; uint32_t t0 = millis();
    while (sent < size) {
        if (!c->pcb || c->state == CONN_CLOSED) break;
        uint16_t space = tcp_sndbuf(c->pcb);
        if (space == 0) {
            eth_pump();
            if (millis() - t0 > 4000) break;
            continue;
        }
        uint16_t n = (uint16_t)((size - sent) < space ? (size - sent) : space);
        err_t we = tcp_write(c->pcb, buf + sent, n, TCP_WRITE_FLAG_COPY);
        if (we != ERR_OK) {
            if (we != ERR_MEM) break;              /* non-transient error: give up */
            eth_pump();
            if (millis() - t0 > 4000) break;       /* bounded like the sndbuf==0 path */
            continue;
        }
        sent += n; t0 = millis();
    }
    if (c->pcb) tcp_output(c->pcb);
    return sent;
}

int EthernetClient::available() { if (!_valid()) return 0; eth_pump(); return eth_conn_available(_sock); }
int EthernetClient::read() { uint8_t b; return (read(&b,1) == 1) ? b : -1; }
int EthernetClient::read(uint8_t *buf, size_t size) { if (!_valid()) return -1; eth_pump(); return eth_conn_read(_sock, buf, (int)size); }
int EthernetClient::peek() { if (!_valid()) return -1; eth_pump(); return eth_conn_peek(_sock); }
void EthernetClient::flush() { if (_valid() && eth_conns[_sock].pcb) { tcp_output(eth_conns[_sock].pcb); eth_pump(); } }

void EthernetClient::stop() {
    if (!_valid()) { _sock = -1; return; }
    eth_conn_free(_sock);
    eth_pump();                     /* let the FIN go out */
    _sock = -1;
}

uint8_t EthernetClient::connected() {
    if (!_valid()) return 0;
    eth_pump();
    tcp_conn_t *c = &eth_conns[_sock];
    if (c->state == CONN_ESTABLISHED) return 1;
    return eth_conn_available(_sock) > 0 ? 1 : 0;   /* data buffered past a peer FIN */
}

EthernetClient::operator bool() { return _valid(); }
