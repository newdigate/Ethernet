#ifndef ethernetserver_h_
#define ethernetserver_h_
#include "Server.h"
class EthernetClient;
class EthernetServer : public Server {
public:
    explicit EthernetServer(uint16_t port) : _port(port), _listen(nullptr) {}
    void begin() override;
    EthernetClient available();
    EthernetClient accept();
    size_t write(uint8_t b) override;
    size_t write(const uint8_t *buf, size_t size) override;
    using Print::write;
    uint16_t _port;
    void *_listen;               /* struct tcp_pcb* (opaque here) */
};
#endif
