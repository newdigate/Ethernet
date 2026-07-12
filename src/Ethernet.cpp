#include "Ethernet.h"
#include "utility/socket_lwip.h"
#include "lwip/init.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/netif.h"
#include "netif/ethernet.h"
#include "ethernetif.h"

extern "C" { uint8_t g_mac[6] = {0x02,0x00,0x00,0x00,0x00,0x01}; }   /* ethernetif.c extern-refs this */
/* eth_conns lives in socket_lwip.cpp (single definition; declared extern in
 * socket_lwip.h). Defining it here too caused a link-time "multiple definition". */
extern "C" uint16_t enet_mdio_read(uint8_t phy, uint8_t reg);   /* frozen core enet.c:468 */
EthernetClass Ethernet;

static IPAddress from_ip4(const ip4_addr_t *a) {
    uint32_t v = a ? a->addr : 0;  /* network byte order */
    return IPAddress((v)&0xff,(v>>8)&0xff,(v>>16)&0xff,(v>>24)&0xff);
}
static void to_ip4(IPAddress in, ip4_addr_t *out) { IP4_ADDR(out, in[0], in[1], in[2], in[3]); }

void EthernetClass::netif_bringup(uint8_t *mac, const ip4_addr_t *ip, const ip4_addr_t *nm, const ip4_addr_t *gw) {
    for (int i=0;i<6;i++) g_mac[i]=mac[i];
    if (!_inited) {
        lwip_init();
        srand((unsigned)(millis() ^ (g_mac[5] << 8) ^ g_mac[4]));  /* best-effort seed for LWIP_RAND (DNS TXID) */
        _inited = true;
    }
    struct netif *n = eth_netif();
    if (!_netif_added) {
        netif_add(n, ip, nm, gw, NULL, ethernetif_init, ethernet_input);
        netif_set_default(n);
        _netif_added = true;
    } else {
        netif_set_addr(n, ip, nm, gw);      /* re-begin: reconfigure, do NOT re-add */
    }
    netif_set_up(n);
}

int EthernetClass::begin(uint8_t *mac, unsigned long timeout, unsigned long responseTimeout) {
    (void)responseTimeout;
    ip4_addr_t any; ip4_addr_set_zero(&any);
    netif_bringup(mac, &any, &any, &any);
    dhcp_start(eth_netif());
    uint32_t t0 = millis();
    while (!dhcp_supplied_address(eth_netif())) {
        eth_pump();
        if (millis() - t0 > timeout) return 0;
    }
    return 1;
}

void EthernetClass::begin(uint8_t *mac, IPAddress ip) {
    begin(mac, ip, IPAddress(ip[0],ip[1],ip[2],1));                    /* dns = gw default */
}
void EthernetClass::begin(uint8_t *mac, IPAddress ip, IPAddress dns) {
    begin(mac, ip, dns, IPAddress(ip[0],ip[1],ip[2],1));
}
void EthernetClass::begin(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway) {
    begin(mac, ip, dns, gateway, IPAddress(255,255,255,0));
}
void EthernetClass::begin(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway, IPAddress subnet) {
    ip4_addr_t a,nm,gw; to_ip4(ip,&a); to_ip4(subnet,&nm); to_ip4(gateway,&gw);
    netif_bringup(mac, &a, &nm, &gw);
    setDnsServerIP(dns);
}

int  EthernetClass::maintain() { eth_pump(); return 0; }    /* DHCP renew is automatic via timeouts */
EthernetLinkStatus EthernetClass::linkStatus() {
    (void)enet_mdio_read(3, 1);                       /* BMSR latches link-down low; read twice */
    return (enet_mdio_read(3, 1) & 0x0004) ? LinkON : LinkOFF;
}
EthernetHardwareStatus EthernetClass::hardwareStatus() { return EthernetOther; }
IPAddress EthernetClass::localIP()    { return from_ip4(netif_ip4_addr(eth_netif())); }
IPAddress EthernetClass::subnetMask() { return from_ip4(netif_ip4_netmask(eth_netif())); }
IPAddress EthernetClass::gatewayIP()  { return from_ip4(netif_ip4_gw(eth_netif())); }
IPAddress EthernetClass::dnsServerIP(){ const ip_addr_t *d = dns_getserver(0); return from_ip4(ip_2_ip4(d)); }
void EthernetClass::setDnsServerIP(const IPAddress dns) { ip_addr_t d; IP_ADDR4(&d,dns[0],dns[1],dns[2],dns[3]); dns_setserver(0,&d); }
void EthernetClass::MACAddress(uint8_t *m) { for (int i=0;i<6;i++) m[i]=g_mac[i]; }
void EthernetClass::setMACAddress(const uint8_t *m) { for (int i=0;i<6;i++) g_mac[i]=m[i]; }
void EthernetClass::loop() { eth_pump(); }
