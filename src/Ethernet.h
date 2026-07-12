#ifndef ethernet_h_
#define ethernet_h_
#include <Arduino.h>
#include "IPAddress.h"
#include "Client.h"
#include "Server.h"
#include "Udp.h"
#include "lwip/ip_addr.h"   /* ip4_addr_t used in netif_bringup() below (fix: not
                                pulled in transitively by any header above) */

enum EthernetLinkStatus { Unknown, LinkON, LinkOFF };
enum EthernetHardwareStatus { EthernetNoHardware, EthernetW5100, EthernetW5200, EthernetW5500, EthernetOther };

class EthernetClass {
public:
    int  begin(uint8_t *mac, unsigned long timeout = 60000, unsigned long responseTimeout = 4000);
    void begin(uint8_t *mac, IPAddress ip);
    void begin(uint8_t *mac, IPAddress ip, IPAddress dns);
    void begin(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway);
    void begin(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway, IPAddress subnet);
    int  maintain();
    EthernetLinkStatus linkStatus();
    EthernetHardwareStatus hardwareStatus();
    IPAddress localIP();
    IPAddress subnetMask();
    IPAddress gatewayIP();
    IPAddress dnsServerIP();
    void setDnsServerIP(const IPAddress dns);
    void MACAddress(uint8_t *mac_address);
    void setMACAddress(const uint8_t *mac_address);
    void loop();                       /* pump wrapper for sketches (optional) */
private:
    void netif_bringup(uint8_t *mac, const ip4_addr_t *ip, const ip4_addr_t *nm, const ip4_addr_t *gw);
    bool _inited = false;
};
extern EthernetClass Ethernet;
#include "EthernetClient.h"
#include "EthernetServer.h"
#include "EthernetUdp.h"
#endif
