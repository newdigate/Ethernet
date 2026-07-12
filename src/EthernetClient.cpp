#include "EthernetClient.h"

/* Trivial stubs for the boot gate (Task 2). Real bodies land in Task 3 (RX/TX
 * bridge) and Task 5 (connect). */

int EthernetClient::connect(IPAddress ip, uint16_t port) { (void)ip; (void)port; return 0; }
int EthernetClient::connect(const char *host, uint16_t port) { (void)host; (void)port; return 0; }
size_t EthernetClient::write(uint8_t b) { (void)b; return 0; }
size_t EthernetClient::write(const uint8_t *buf, size_t size) { (void)buf; (void)size; return 0; }
int EthernetClient::available() { return 0; }
int EthernetClient::read() { return -1; }
int EthernetClient::read(uint8_t *buf, size_t size) { (void)buf; (void)size; return -1; }
int EthernetClient::peek() { return -1; }
void EthernetClient::flush() { }
void EthernetClient::stop() { }
uint8_t EthernetClient::connected() { return 0; }
EthernetClient::operator bool() { return false; }
