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


#define TCP_CB_TABLE_SIZE 32i

#define TCP_FLG_FIN 0x01
#define TCP_FLG_SYN 0x02
#define TCP_FLG_RST 0x04
#define TCP_FLG_PSH 0x08
#define TCP_FLG_ACK 0x10
#define TCP_FLG_URG 0x20

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
	uint8_t window[65535];
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
			if (!fcb) {// frecuency cb.
				fcb = entry;
			}
		}
		else if ((!cb->ifs || cb->ifs == ifs) && cb->port == hdr->dest_port) { //check ifs and port.
			if ((cb->peer.addr == src) && (cb->peer.port == hdr->src_port)) {//check peer.
				break;
			}
			if (cb->state == LISTEN && !lcb) {//listen cb.
				lcb = cb;
			}
		}
	}
	if (cb == fin) {//not hit active cb.
		if (!lcb || !fcb || !(hdr->flg & TCP_FLAG_SYN)) {
			//send RST
			return;
		}
		cb = fcb;
		cb->used = 1;
		cb->state = lcb->state;
		cb->ifs = ifs;
		cb->port = lcb->port;
		cb->peer.addr = src;
		cb->peer.port = port;
		cb->rcv.wnd = sizeof(cb->window); // ????????
		cb->parent = lcb;

	}
	//event

	return;
}

void 
tx_tcp(struct tcp_cb_entry *cb, uint32_t seq, uint32_t ack, uint8_t flag, struct rte_mbuf *mbuf, uint8_t *data, uint32_t size) {
	uint8_t segment[1500];
	struct tcp_hdr *tcphdr;
	uint32_t self, peer;
	//uint32_t pseudo = 0;
	
	memset(sengment, 0, sizeof(segment));
	tcphdr = (struct tcp_hdr *)segment;
	tcphdr->src_port = cb->port;
	tcphdr->dest_port = cb->peer.port;
	tcphdr->seqence = htonl(seq);
	tcphdr->acknowledge = htonl(ack);
	tcphdr->offset = (sizeof(struct tcp_hdr) >> 2) << 4;
	tcphdr->flag = flag;
	tcphdr->window_size = htons(cb->recv.wnd); ////////
	tcphdr->checksum = 0;
	tcphdr->urg = 0;

}
