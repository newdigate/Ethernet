#include "EthernetUdp.h"

/* Trivial stubs for the boot gate (Task 2). Real bodies land in Task 4. */

EthernetUDP::EthernetUDP() : _pcb(nullptr), _rxh(0), _rxt(0), _cur(nullptr), _cur_off(0), _txlen(0) {}

uint8_t EthernetUDP::begin(uint16_t port) { (void)port; return 0; }
void EthernetUDP::stop() { }
int EthernetUDP::beginPacket(IPAddress ip, uint16_t port) { (void)ip; (void)port; return 0; }
int EthernetUDP::beginPacket(const char *host, uint16_t port) { (void)host; (void)port; return 0; }
int EthernetUDP::endPacket() { return 0; }
size_t EthernetUDP::write(uint8_t b) { (void)b; return 0; }
size_t EthernetUDP::write(const uint8_t *buf, size_t size) { (void)buf; (void)size; return 0; }
int EthernetUDP::parsePacket() { return 0; }
int EthernetUDP::available() { return 0; }
int EthernetUDP::read() { return -1; }
int EthernetUDP::read(unsigned char *buf, size_t len) { (void)buf; (void)len; return -1; }
int EthernetUDP::read(char *buf, size_t len) { (void)buf; (void)len; return -1; }
int EthernetUDP::peek() { return -1; }
void EthernetUDP::flush() { }
IPAddress EthernetUDP::remoteIP() { return IPAddress(0,0,0,0); }
uint16_t EthernetUDP::remotePort() { return 0; }
