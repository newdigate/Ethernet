#include "EthernetServer.h"
#include "EthernetClient.h"

/* Trivial stubs for the boot gate (Task 2). Real bodies (listen + accept-pool)
 * land in Task 3. */

void EthernetServer::begin() { }
EthernetClient EthernetServer::available() { return EthernetClient(-1); }
EthernetClient EthernetServer::accept() { return EthernetClient(-1); }
size_t EthernetServer::write(uint8_t b) { (void)b; return 0; }
size_t EthernetServer::write(const uint8_t *buf, size_t size) { (void)buf; (void)size; return 0; }
