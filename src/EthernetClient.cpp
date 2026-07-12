#include "Ethernet.h"
#include "EthernetClient.h"
#include "utility/socket_lwip.h"
#include "lwip/tcp.h"

int EthernetClient::connect(IPAddress ip, uint16_t port) { (void)ip;(void)port; return 0; } /* Task 5 */
int EthernetClient::connect(const char *host, uint16_t port) { (void)host;(void)port; return 0; } /* Task 6 */

size_t EthernetClient::write(uint8_t b) { return write(&b, 1); }

size_t EthernetClient::write(const uint8_t *buf, size_t size) {
    if (_sock < 0) return 0;
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
        if (tcp_write(c->pcb, buf + sent, n, TCP_WRITE_FLAG_COPY) != ERR_OK) { eth_pump(); continue; }
        sent += n; t0 = millis();
    }
    if (c->pcb) tcp_output(c->pcb);
    return sent;
}

int EthernetClient::available() { if (_sock < 0) return 0; eth_pump(); return eth_conn_available(_sock); }
int EthernetClient::read() { uint8_t b; return (read(&b,1) == 1) ? b : -1; }
int EthernetClient::read(uint8_t *buf, size_t size) { if (_sock < 0) return -1; eth_pump(); return eth_conn_read(_sock, buf, (int)size); }
int EthernetClient::peek() { if (_sock < 0) return -1; eth_pump(); return eth_conn_peek(_sock); }
void EthernetClient::flush() { if (_sock >= 0 && eth_conns[_sock].pcb) { tcp_output(eth_conns[_sock].pcb); eth_pump(); } }

void EthernetClient::stop() {
    if (_sock < 0) return;
    eth_conn_free(_sock);
    eth_pump();                     /* let the FIN go out */
    _sock = -1;
}

uint8_t EthernetClient::connected() {
    if (_sock < 0) return 0;
    eth_pump();
    tcp_conn_t *c = &eth_conns[_sock];
    if (c->state == CONN_ESTABLISHED) return 1;
    return eth_conn_available(_sock) > 0 ? 1 : 0;   /* data buffered past a peer FIN */
}

EthernetClient::operator bool() { return _sock >= 0 && eth_conns[_sock].state != CONN_FREE; }
