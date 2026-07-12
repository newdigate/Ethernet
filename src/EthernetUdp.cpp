#include "Ethernet.h"
#include "EthernetUdp.h"
#include "utility/socket_lwip.h"
#include "lwip/udp.h"

static void eth_udp_recv_cb(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                            const ip_addr_t *addr, u16_t port) {
    (void)pcb;
    EthernetUDP *self = (EthernetUDP *)arg;
    self->_enqueue(p, addr, port);
}

/* helper on the class (declare in EthernetUdp.h private: void _enqueue(...);) */
void EthernetUDP::_enqueue(void *p, const ip_addr_t *addr, uint16_t port) {
    int nt = (_rxt + 1) % RXQ;
    if (nt == _rxh) { pbuf_free((struct pbuf*)p); return; }   /* ring full: drop */
    _rxq[_rxt].p = p; _rxq[_rxt].ip = *addr; _rxq[_rxt].port = port; _rxt = nt;
}

EthernetUDP::EthernetUDP() : _pcb(nullptr), _rxh(0), _rxt(0), _cur(nullptr), _cur_off(0), _txlen(0) {}

uint8_t EthernetUDP::begin(uint16_t port) {
    struct udp_pcb *pcb = udp_new();
    if (!pcb) return 0;
    if (udp_bind(pcb, IP_ANY_TYPE, port) != ERR_OK) { udp_remove(pcb); return 0; }
    udp_recv(pcb, eth_udp_recv_cb, this);
    _pcb = pcb; return 1;
}

void EthernetUDP::stop() {
    if (_cur) { pbuf_free((struct pbuf*)_cur); _cur = nullptr; }
    while (_rxh != _rxt) { pbuf_free((struct pbuf*)_rxq[_rxh].p); _rxh = (_rxh+1)%RXQ; }
    if (_pcb) { udp_remove((struct udp_pcb*)_pcb); _pcb = nullptr; }
}

int EthernetUDP::parsePacket() {
    eth_pump();
    if (_cur) { pbuf_free((struct pbuf*)_cur); _cur = nullptr; _cur_off = 0; }
    if (_rxh == _rxt) return 0;
    _cur = _rxq[_rxh].p; _rip = _rxq[_rxh].ip; _rport = _rxq[_rxh].port;
    _rxh = (_rxh + 1) % RXQ; _cur_off = 0;
    return (int)((struct pbuf*)_cur)->tot_len;
}

int EthernetUDP::available() { return _cur ? (int)(((struct pbuf*)_cur)->tot_len - _cur_off) : 0; }

int EthernetUDP::read(unsigned char *buf, size_t len) {
    if (!_cur) return -1;
    struct pbuf *p = (struct pbuf*)_cur;
    uint16_t avail = p->tot_len - _cur_off; if (avail == 0) return -1;
    uint16_t n = (uint16_t)(len < avail ? len : avail);
    pbuf_copy_partial(p, buf, n, _cur_off); _cur_off += n;
    if (_cur_off >= p->tot_len) { pbuf_free(p); _cur = nullptr; _cur_off = 0; }
    return n;
}
int EthernetUDP::read(char *buf, size_t len) { return read((unsigned char*)buf, len); }
int EthernetUDP::read() { unsigned char b; return (read(&b,1)==1)? b : -1; }
int EthernetUDP::peek() { if(!_cur) return -1; struct pbuf*p=(struct pbuf*)_cur; if(_cur_off>=p->tot_len) return -1; return pbuf_get_at(p,_cur_off); }
void EthernetUDP::flush() { if (_cur) { pbuf_free((struct pbuf*)_cur); _cur=nullptr; _cur_off=0; } }

int EthernetUDP::beginPacket(IPAddress ip, uint16_t port) {
    IP_ADDR4(&_dip, ip[0],ip[1],ip[2],ip[3]); _dport = port; _txlen = 0; return 1;
}
int EthernetUDP::beginPacket(const char *host, uint16_t port) { (void)host;(void)port; return 0; } /* Task 6 */

size_t EthernetUDP::write(uint8_t b) { return write(&b, 1); }
size_t EthernetUDP::write(const uint8_t *buf, size_t size) {
    uint16_t room = sizeof(_txbuf) - _txlen; uint16_t n = (uint16_t)(size < room ? size : room);
    memcpy(_txbuf + _txlen, buf, n); _txlen += n; return n;
}
int EthernetUDP::endPacket() {
    if (!_pcb) return 0;
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, _txlen, PBUF_RAM);
    if (!p) return 0;
    pbuf_take(p, _txbuf, _txlen);
    err_t e = udp_sendto((struct udp_pcb*)_pcb, p, &_dip, _dport);
    pbuf_free(p); eth_pump();
    return e == ERR_OK ? 1 : 0;
}
IPAddress EthernetUDP::remoteIP() { uint32_t v = ip_2_ip4(&_rip)->addr; return IPAddress(v&0xff,(v>>8)&0xff,(v>>16)&0xff,(v>>24)&0xff); }
uint16_t EthernetUDP::remotePort() { return _rport; }
