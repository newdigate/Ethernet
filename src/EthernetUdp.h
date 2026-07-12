#ifndef ethernetudp_h_
#define ethernetudp_h_
#include <string.h>
#include "Udp.h"
#include "lwip/ip_addr.h"   /* ip_addr_t used in the private members below (fix: not
                                pulled in transitively by Udp.h) */
class EthernetUDP : public UDP {
public:
    EthernetUDP();
    uint8_t begin(uint16_t port) override;
    void stop() override;
    int beginPacket(IPAddress ip, uint16_t port) override;
    int beginPacket(const char *host, uint16_t port) override;
    int endPacket() override;
    size_t write(uint8_t b) override;
    size_t write(const uint8_t *buf, size_t size) override;
    int parsePacket() override;
    int available() override;
    int read() override;
    int read(unsigned char *buf, size_t len) override;
    int read(char *buf, size_t len) override;
    int peek() override;
    void flush() override;
    IPAddress remoteIP() override;
    uint16_t remotePort() override;
    using Print::write;
private:
    void *_pcb;                  /* struct udp_pcb* */
    struct udp_slot { void *p; ip_addr_t ip; uint16_t port; };
    static const int RXQ = 4;
    udp_slot _rxq[RXQ]; int _rxh, _rxt;
    void *_cur; uint16_t _cur_off;
    ip_addr_t _rip; uint16_t _rport;
    ip_addr_t _dip; uint16_t _dport;
    uint8_t _txbuf[1472]; uint16_t _txlen;
public:
    void _enqueue(void *p, const ip_addr_t *addr, uint16_t port);
};
#endif
