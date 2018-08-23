#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<pthread.h>

#include<rte_mbuf.h>

#include"include/ip.h"
#include"include/ethernet.h"
#include"include/lunetta.h"

#define IP_INTERFACE_NUM 1
#define ROUTE_TABLE_SIZE 3

#define IP_HEADER_LEN 20

const uint32_t ip_broadcast = 0xffffffff;

uint32_t get_ip_addr(struct ether_port *port) {
	return 0x0a000005;
}

struct ip_interface {//one interface
	struct ether_port *port;
	uint32_t addr;
	uint32_t mask;
	uint32_t network;
	uint32_t broadcast;
};
struct ip_interface interfaces[IP_INTERFACE_NUM];

struct route_node {
	uint8_t used;
	struct ether_port *port;
	uint32_t network;
	uint32_t mask;
	uint32_t next;
};
struct route_node route_table[ROUTE_TABLE_SIZE];

void ip_interfaces_dump() {
	struct ip_interface *ifs;
	struct ip_interface *fin = interfaces + IP_INTERFACE_NUM;

	printf("===== ip_interfaces =====\n");
	for (ifs = interfaces; ifs != fin; ifs++) {
		printf("--------------------------\n");
		printf("port: %p(pointer)\n", ifs->port);
		printf("addr: %x\n", ifs->addr);
		printf("mask: %x\n", ifs->mask);
		printf("network: %x\n", ifs->network);
		printf("broadcast: %x\n", ifs->broadcast);
	}
	printf("=========================\n");
}

void route_table_dump() {
	struct route_node *node	;
	struct route_node *fin = route_table + ROUTE_TABLE_SIZE;
	
	printf("===== ip_route_table =====\n");
	for (node = route_table; node != fin; node++) {
		printf("----------------------------\n");
		printf("used: %u\n", node->used);
		printf("port: %p(pointer)\n", node->port);
		printf("network: %x\n", node->network);
		printf("mask: %x\n", node->mask);
		printf("next: %x\n", node->next);
	}
	printf("==========================\n");
}

void 
route_table_init() {
	struct route_node *route;
	struct route_node *fin = route_table + ROUTE_TABLE_SIZE;
	for (route = route_table; route != fin; route++) {
		route->used = 0;
		route->port = NULL;
		route->network = 0;
		route->mask = 0;
		route->next = 0;
	}
	return;
}

int 
route_table_add(const uint32_t network, const uint32_t mask, const uint32_t next, struct ether_port *port) {
	struct route_node *route;
	struct route_node *fin = route_table +  ROUTE_TABLE_SIZE;

	for (route = route_table; route != fin; route++) {
		if (!route->used) {
			route->used = 1;
			route->port = port;
			route->network = network;
			route->mask = mask;
			route->next = next;
			return 0;
		}
	}
	return -1;
}

int
ip_init(struct ip_init_info *info, uint16_t num, struct ether_port *gate_port, uint32_t gateway) {
	struct ip_interface *ifs = interfaces;

	route_table_init();

	if (num > IP_INTERFACE_NUM)
		num = IP_INTERFACE_NUM;

	for (int i = 0; i < num; i++) {
		ifs->port = info->port;
		ifs->addr = info->addr;
		ifs->mask = info->mask;
		ifs->network = info->addr & info->mask;
		ifs->broadcast = ifs->network | ~info->mask;

		if (route_table_add(ifs->network, info->mask, 0, info->port) == -1)
			return -1;
		ifs++;
		info++;
	}
	if (gate_port && gateway) {
		if (route_table_add(0, 0, gateway, gate_port) == -1)
			return -1;
	}

	return 0;
}

int
ip_route_lookup(uint32_t dest, uint32_t *nexthop, struct ip_interface **ifs) {
	*ifs = interfaces;
	return 0;
}

static uint16_t 
ip_generate_id(void) {
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	static uint16_t id = 0;
	uint16_t ret;

	pthread_mutex_lock(&mutex);
	ret = id++;
	pthread_mutex_unlock(&mutex);
	return ret;
}

void
tx_ip_core(uint8_t proto, struct rte_mbuf *mbuf, uint32_t size, uint32_t dest, uint32_t *nexthop, struct ip_interface *ifs) {
	if (size > 1480 || !mbuf || !ifs) {
		fprintf(stderr, "tx_ip_core error\n");
		exit(1);
	}
	uint32_t len = size + IP_HEADER_LEN;

	//not suport option.
	struct ip_hdr *iphdr = (uint8_t *)rte_pktmbuf_prepend(mbuf, sizeof(uint8_t) * IP_HEADER_LEN);
	iphdr->hdr_len = IP_HEADER_LEN >> 2;
	iphdr->version = 4;
	iphdr->type_of_service = 0;
	iphdr->total_len = htons(len);
	iphdr->id = ip_generate_id();
	iphdr->frag = 0;
	iphdr->ttl = 0xff;
	iphdr->proto = proto;
	iphdr->check = 0; 
	iphdr->src_addr = htonl(ifs->addr);
	iphdr->dest_addr = htonl(dest);
	iphdr->check = checksum_s((uint16_t *)iphdr, IP_HEADER_LEN, 0);
	if (*nexthop) {
		*nexthop = htonl(*nexthop);
		printf("nexthop: %x\n",*nexthop);
		tx_ether(ifs->port, mbuf, len, ETHERTYPE_IP, nexthop, NULL);
	}
	else {//broadcast
		tx_ether(ifs->port, mbuf, len, ETHERTYPE_IP, NULL, &ether_broadcast);
	}
	return;
}

struct ip_interface*
match_directed_broadcast(uint32_t dest) {
	struct ip_interface *ifs;
	struct ip_interface *fin = interfaces + IP_INTERFACE_NUM;
	for (ifs = interfaces; ifs != fin; ifs++) {
		if (dest == ifs->broadcast) {
			return ifs;
		}
	}
	return NULL;
}

void 
tx_ip(uint8_t proto, struct rte_mbuf *mbuf, uint32_t size, uint32_t dest, uint32_t src) {
	uint32_t nexthop = 0;
	struct ip_interface *ifs;
	struct ip_interface *fin = interfaces + IP_INTERFACE_NUM;
	printf("*** tx_ip ***\n");

	if (dest == ip_broadcast) {
		printf("limited broadcast\n");
		//lookup src addr
		for (ifs = interfaces; ifs != fin; ifs++) {
			if (src == ifs->addr) {
				//tx_ip_core(proto, mbuf, size, dest, 0, ifs->port);
				break;
			}
			ifs = NULL;
		}
	}
	else if ((ifs = match_directed_broadcast(dest)) == NULL) {
		if (ip_route_lookup(dest, &nexthop, &ifs) == -1) {
			fprintf(stderr, "no route\n");
			exit(1);/*****/
		}
		if (!nexthop) {//same network. nexthop is dest. (now that is set 0.)
			nexthop = dest;
		}
	}
	if (!ifs) {
		fprintf(stderr, "ip interface is not found\n");
		exit(1);
	}
	tx_ip_core(proto, mbuf, size, dest, &nexthop, ifs);
	
	return;
}
