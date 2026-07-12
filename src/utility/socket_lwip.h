#ifndef ETHERNET_SOCKET_LWIP_H
#define ETHERNET_SOCKET_LWIP_H
#include <stdint.h>
#include "lwip/tcp.h"
#include "lwip/netif.h"

#define ETH_MAX_SOCK_NUM 4          /* within lwipopts MEMP_NUM_TCP_PCB=5 */

typedef enum { CONN_FREE=0, CONN_CONNECTING, CONN_ESTABLISHED, CONN_CLOSING, CONN_CLOSED } conn_state_t;

typedef struct {
    struct tcp_pcb *pcb;            /* NULL once lwIP frees it (err_cb) */
    struct pbuf    *rx_head;        /* received-but-unread pbuf chain (we hold it) */
    uint16_t        rx_off;         /* read offset into rx_head */
    volatile conn_state_t state;
    ip_addr_t       remote_ip;
    uint16_t        remote_port;
    uint16_t        accept_port;    /* listening port a server conn arrived on; 0 for outbound */
} tcp_conn_t;

#ifdef __cplusplus
extern "C" {
#endif
extern tcp_conn_t eth_conns[ETH_MAX_SOCK_NUM];
extern uint8_t    g_mac[6];         /* defined in the vendored lwip ethernetif.c */

struct netif *eth_netif(void);      /* the single shared netif (owned by EthernetClass) */
void  eth_pump(void);               /* reentrancy-guarded: ethernetif_poll + sys_check_timeouts */

int   eth_conn_alloc(void);         /* -> index in [0,ETH_MAX_SOCK_NUM) or -1 */
void  eth_conn_free(int i);         /* close pcb if any, free rx pbufs, mark FREE */
void  eth_conn_bind_callbacks(int i);   /* wire recv/sent/err/poll on eth_conns[i].pcb */
int   eth_conn_available(int i);        /* unread bytes in the rx chain */
int   eth_conn_read(int i, uint8_t *buf, int len);  /* drain + tcp_recved; -1 if none */
int   eth_conn_peek(int i);             /* first unread byte or -1 */

/* raw callbacks (exposed so EthernetServer's accept can share them) */
err_t eth_recv_cb(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
err_t eth_sent_cb(void *arg, struct tcp_pcb *pcb, uint16_t len);
void  eth_err_cb(void *arg, err_t err);
#ifdef __cplusplus
}
#endif
#endif
