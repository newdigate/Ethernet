# Ethernet (i.MX RT1176)

The familiar Arduino **`Ethernet`** socket API — `EthernetClient`, `EthernetServer`,
`EthernetUDP`, and the `Ethernet` (`EthernetClass`) singleton — for the NXP
**i.MX RT1176** (MIMXRT1176-EVKB), implemented over the lwIP raw API rather than a
WIZnet shield.

Stock Arduino networking sketches (WebClient, echo servers, UDP tools, libraries
such as PubSubClient) compile and run unchanged:

```cpp
#include <Ethernet.h>
uint8_t mac[6] = {0x02,0,0,0,0,1};
EthernetClient client;
void setup() {
  Ethernet.begin(mac);                 // DHCP
  if (client.connect("example.com", 80)) {
    client.println("GET / HTTP/1.0\r\n");
    while (client.connected())
      if (client.available()) Serial.write(client.read());
  }
}
```

## Design

- Wraps **lwIP** (bare-metal `NO_SYS`, raw callback API) — see
  [newdigate/lwip](https://github.com/newdigate/lwip). A synchronous Arduino facade
  sits over the async lwIP callbacks, driven by a cooperative, reentrancy-guarded
  pump; because the stack is single-threaded, the bridge needs no locks.
- A shared connection pool (sockindex model, with reap + a generation guard),
  the hold-pbuf / `tcp_recved`-on-drain RX path, a UDP datagram ring, and DNS.
- Layers on a hand-rolled L2 ENET driver + the lwIP `NO_SYS` port; the core is
  frozen and untouched.

**Scope:** `EthernetClient`/`EthernetServer`/`EthernetUDP` + `EthernetClass`
(DHCP + static config, `localIP`/`linkStatus`/`maintain`) with DNS. UDP multicast
is not implemented.

## License

MIT (see [LICENSE](LICENSE)). The `IPAddress`/`Client`/`Server`/`UDP` base classes
and the public API shape are incorporated from the MIT-licensed Arduino Ethernet
library and Teensyduino core (© Paul Stoffregen / Arduino LLC); those files keep
their original headers.
