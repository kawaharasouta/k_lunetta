#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<arpa/inet.h>//byte order

#include <rte_mbuf.h>
#include <rte_ether.h>
#include<rte_memcpy.h>

#include"include/lunetta.h"
#include"include/pkt_io.h"
#include"include/ethernet.h"

#define ETHER_PAYLOAD_MAX_LEN 1500
#define ETHER_FRAME_MIN_LEN 64
#define ETHER_HEADER_LEN 14

#define ETHER_PORT_MAX_NUM 1

void hexdump(uint8_t *buf, int size);

struct ether_port ports[ETHER_PORT_MAX_NUM];

ethernet_addr ether_broadcast = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/* Assumed to be called after dpdk_init.     *
 * Load the port_config setting.             *
 * For now, port_config is a single pointer. */
int
ethernet_init(struct port_config *port, uint16_t num) {
	if (num > ETHER_PORT_MAX_NUM)
		num = ETHER_PORT_MAX_NUM;
	for (int i = 0; i < ETHER_PORT_MAX_NUM; i++) {
		ports[i].port_num = port->port_num;
		ports[i].mac_addr = port->mac_addr;
	}
}
struct ether_port*
get_port_pointer() {
	return ports;
}
struct ether_port*
find_port_pointer(uint16_t port_num) {
	for (int i = 0; i < ETHER_PORT_MAX_NUM; i++) {
		if (ports[i].port_num == port_num)
			return &ports[i];
	}
}

int 
is_zero_ethernet_addr(ethernet_addr *addr) {
	for (int i = 0; i < ETHER_ADDR_LEN; i++) {
		if (addr->addr[i] != 0)
			return 0;
	}
	return 1;
}

void 
print_mac_addr(ethernet_addr *addr) {
	printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
	addr->addr[0], addr->addr[1], addr->addr[2], addr->addr[3], addr->addr[4], addr->addr[5]);
}

int 
is_equal_mac_addr(ethernet_addr *addr1, ethernet_addr *addr2) {
	for (int i = 0; i < ETHER_ADDR_LEN; i++) {
		if (addr1->addr[i] != addr2->addr[i])
			return 0;
	}
	return 1;
}

int 
is_ether_broadcast(ethernet_addr *addr) {
	if (!addr)
		return 0;
	for (int i = 0; i < ETHER_ADDR_LEN; i++) {
		if (addr->addr[i] != 0xff)
			return 0;
	}
	return 1;
}

void 
print_ethernet_hdr(struct ethernet_hdr *ether_hdr) {
	printf("ether dest addr: ");
	print_mac_addr(&ether_hdr->dest);
	printf("ether src addr: ");
	print_mac_addr(&ether_hdr->src);
	printf("type: %04x\n", ntohs(ether_hdr->type));
	return;
}

void 
tx_ether(struct ether_port *port, struct rte_mbuf *mbuf, uint32_t size, uint16_t type, const void *paddr, ethernet_addr *dest) {
	int ret;
	uint32_t len;/* = 64;*/
	uint32_t paddr_;
	ethernet_addr haddr;
	struct ethernet_hdr *eth;

	if (size > ETHER_PAYLOAD_MAX_LEN || !mbuf || (!paddr && !dest)) {
		return;
	}

	uint8_t *p = rte_pktmbuf_mtod(mbuf, uint8_t*);
	/* For the time being, put the broadcast addr on */
	if (paddr) {
		//*paddr = (void)htonl((uint32_t)*paddr);
		//paddr_ = (uint32_t)*paddr;
		//paddr_ = htonl(paddr_);
		ret = arp_resolve(port, paddr, &haddr, (void *)mbuf, size);
		//for (int i = 0; i < ETHER_ADDR_LEN; i++) {
		//	haddr.addr[i] = ether_broadcast.addr[i];
		//}
		if (ret != 1)
			return;

		//before receiving arp reply. resend arp_req. do not consider data loss.
		if (is_zero_ethernet_addr(&haddr)) {
			send_req(port, paddr);
			rte_pktmbuf_free(mbuf);
			return;
		}
	}
	else {
		for (int i = 0; i < ETHER_ADDR_LEN; i++) {
			haddr.addr[i] = dest->addr[i];
		}
	}

	uint8_t *pp = (uint8_t *)rte_pktmbuf_prepend(mbuf, sizeof(uint8_t) * ETHER_HEADER_LEN);
	uint8_t *ppp = pp;
	if (!pp) {
		printf("mbuf prepend error\n");
		return;
	}
	eth = (struct ethernet_hdr *)pp;
	rte_memcpy(eth->dest.addr, haddr.addr, ETHER_ADDR_LEN);
	rte_memcpy(eth->src.addr, port->mac_addr.addr, ETHER_ADDR_LEN);
	eth->type = htons(type);
	len = sizeof(struct ethernet_hdr) + size;
	if (len < ETHER_FRAME_MIN_LEN) {
		pp += len;
		memset(pp, 0x00, ETHER_FRAME_MIN_LEN - len);
		len = ETHER_FRAME_MIN_LEN;
	}

//	printf("***** tx_ether *****\n");
//	hexdump(ppp, len);
//	printf("********************\n");

	/********/
	mbuf->pkt_len = len;
	mbuf->data_len = len;
	tx_pkt(port, &mbuf, 1);
	/********/

	return;
}

void 
rx_ether(struct ether_port *port, struct rte_mbuf *mbuf, uint8_t *data, uint32_t size) {
	printf("size: %d\n", size);
	if (size > 1514)
		return;

	//uint32_t *p = rte_pktmbuf_mtod(mbuf, uint8_t*);
	struct ethernet_hdr *packet = (struct ethernet_hdr *)data;
	uint8_t *pp = data;

	pp += sizeof(struct ethernet_hdr);
	size -= sizeof(struct ethernet_hdr);

//	print_ethernet_hdr(packet);
//	printf("port mac addr:");
//	print_mac_addr(&port->mac_addr);
	
	if (is_equal_mac_addr(&port->mac_addr, &packet->dest) || is_ether_broadcast(&packet->dest)) {
		switch (ntohs(packet->type)) {
			case ETHERTYPE_IP:
			{
				printf("ip\n");
				rx_ip(port, mbuf, (uint8_t *)pp, size);
				break;
			}
			case ETHERTYPE_ARP:
			{
				printf("arp\n");
				rx_arp(port, mbuf, (uint8_t *)pp, size);
				break;
			}
			default:
			{
				printf("wow\n");
				break;
			}
		}
	}
	else {//different mac addr
		printf("different mac addr\n");
	}
	return;
}




void hexdump(uint8_t *buf, int size) {
	int i;
	for (i = 0;i < size; i++){
		fprintf(stdout, "%02x ", *(buf + i));
		if ((i + 1) % 8 == 0){
			fprintf(stdout, "\n");
		}
	}
	fprintf(stdout, "\nfin\n");
}
