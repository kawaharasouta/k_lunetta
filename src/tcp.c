#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<pthread.h>
#include<arpa/inet.h>

#include<rte_mbuf.h>


#include"include/tcp.h"
#include"include/ip.h"
#include"include/queue.h"


#define TCP_CB_TABLE_SIZE 32

typedef enum enm1 {
	CLOSED, //s,c
	LISTEN, //s
	SYN_SEND, //c
	SYN_RCVD, //s
	ESTABLISHED, //s,c
	FIN_WAIT1, //c
	FIN_WAIT2, //c
	CLOSE, //c
	TIME_WAIT, //c
	CLOSE_WAIT, //s
	LAST_ACK, //s
} tcp_state;


struct tcp_cb_entry {
	uint8_t used;
	tcp_state state;
	struct iip_interface *ifs;
	uint16_t port;
	struct {
		uint32_t addr;
		uint16_t port;
	} peer;
	//struct {
	//	uint32_t nxt;
	//	uint32_t una;
	//	uint32_t up;
	//	uint32_t wl1;
	//	uint32_t wl2;
	//	uint32_t wnd;
	//} send;
	//uint32_t iss;
	//struct {
	//	uint32_t nxt;
	//	uint16_t up;
	//	uint16_t wnd;
	//} recv;
	//uint32_t irs;
	//struct ...
	//uint8_t window[65535];
	//struct tcp_cb_entry *parent;
	//struct ...
	//pthread_cond_t 
}; //tcp control brock entry.

struct {
	//pthread_t thread;
	struct tcp_cb_entry table[TCP_CB_TABLE_SIZE];
	//pthread_mutex_t mutex;
} tcp_cb;




void 
rx_tcp(struct rte_mbuf *mbuf, uint8_t *data, uint32_t size, uint32_t src, uint32_t dest, struct ip_interface *ifs) {
	struct tcp_hdr *hdr;
	//uint32_t pseudo;
	struct tcp_cb_entry *entry, *fcb = NULL, *lcb = NULL;
	struct tcp_cb_entry *fin = tcp_cb.table + TCP_CB_TABLE_SIZE;

	hdr = data

	//if(dest != )
	if(size < sizeof(struct tcp_hdr)) {
		return;
	}

	//pseudo and checksum
	//
	//
	
	for (entry = tcp_cb.table; entry != fin; entry++) {
		if (!entry->used) {
			if (!fcb) {
				fcb = entry;
			}
		}
		else if ((!cb->ifs || cb->iface == ifs) && cb->port == hdr->dest_port) {
			if ((cb->peer.addr == src) && (cb->peer.port == hdr->src_port)) {
				break;
			}
			if (cb->state == LISTEN && !lcb) {
				lcb = cb;
			}
		}
	}


}

void 
tx_tcp() {

}
