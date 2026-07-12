#ifndef ethernetclient_h_
#define ethernetclient_h_
#include "Client.h"
class EthernetClient : public Client {
public:
    EthernetClient();
    explicit EthernetClient(int s);
    int connect(IPAddress ip, uint16_t port) override;
    int connect(const char *host, uint16_t port) override;
    size_t write(uint8_t b) override;
    size_t write(const uint8_t *buf, size_t size) override;
    int available() override;
    int read() override;
    int read(uint8_t *buf, size_t size) override;
    int peek() override;
    void flush() override;
    void stop() override;
    uint8_t connected() override;
    operator bool() override;
    using Print::write;
    int _sock;
    uint16_t _gen;
private:
    bool _valid() const;
};
#endif
