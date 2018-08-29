#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<pthread.h>
#include<arpa/inet.h>

#include<rte_mbuf.h>

#include"include/ip.h"
#include"include/ethernet.h"
#include"include/udp.h"
#include"include/lunetta.h"

#define IP_INTERFACE_NUM 1
#define ROUTE_TABLE_SIZE 3

#define IP_HEADER_LEN 20

//#define IP_PROTO_ICMP 1
//#define IP_PROTO_TCP 6
//#define IP_PROTO_UDP 17

const uint32_t ip_broadcast = 0xffffffff;


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

struct ip_interface*
get_ip_interface_from_ether_port(struct ether_port *port) {
	struct ip_interface *ifs;
	struct ip_interface *fin = interfaces + IP_INTERFACE_NUM;
	for (ifs = interfaces; ifs != fin; ifs++) {
		if (ifs->port == port)
			return ifs;
	}
	return NULL;
}

struct ip_interface*
get_ip_interface_from_addr(uint32_t addr) {
	struct ip_interface *ifs;
	struct ip_interface *fin = interfaces + IP_INTERFACE_NUM;
	for (ifs = interfaces; ifs != fin; ifs++) {
		if (ifs->addr == addr)
			return ifs;
	}
	return NULL;
}

uint32_t get_ip_addr(struct ether_port *port) {
	struct ip_interface *ifs = get_ip_interface_from_ether_port(port);
	return ifs->addr;
}

int
ip_route_lookup(uint32_t dest, uint32_t *nexthop, struct ip_interface **ifs) {
	//*ifs = interfaces;
	struct route_node *route, *ret = NULL;
	struct route_node *fin = route_table + ROUTE_TABLE_SIZE;
	for (route = route_table; route != fin; route++) {
		if (route->used && (dest & route->mask) == route->network) {//same network
			if (!ret || ntohl(ret->mask) < ntohl(route->mask)) {//longest match
				ret = route;
			}
		}
	}
	if (!ret) {
		return -1;
	}
	*nexthop = ret->next;
	*ifs = get_ip_interface_from_ether_port(ret->port);

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
		//printf("nexthop: %x\n",*nexthop);
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
tx_ip(uint8_t proto, struct rte_mbuf *mbuf, uint32_t size, uint32_t dest, struct ip_interface *ifs) {
	uint32_t nexthop = 0;
//	struct ip_interface *ifs;
//	struct ip_interface *fin = interfaces + IP_INTERFACE_NUM;

//	if (dest == ip_broadcast) {
//		//printf("limited broadcast\n");
//		//lookup src addr
//		for (ifs = interfaces; ifs != fin; ifs++) {
//			if (src == ifs->addr) {
//				//tx_ip_core(proto, mbuf, size, dest, 0, ifs->port);
//				break;
//			}
//			ifs = NULL;
//		}
//	}
//	else if ((ifs = match_directed_broadcast(dest)) == NULL) {
//		if (ip_route_lookup(dest, &nexthop, &ifs) == -1) {
//			fprintf(stderr, "no route\n");
//			exit(1);/*****/
//		}
//		if (!nexthop) {//same network. nexthop is dest. (now that is set 0.)
//			nexthop = dest;
//		}
//	}

	if (ifs && dest == ip_broadcast) {
		nexthop = dest;
	}
	else {
		if (ip_route_lookup(dest, &nexthop, &ifs) == -1) {
			fprintf(stderr, "no rtoue\n");
			return;
		}
		if (!nexthop) {
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

void 
rx_ip(struct ether_port *port, struct rte_mbuf *mbuf, uint8_t *data, uint32_t size) {
	struct ip_hdr *iphdr = data;
	uint16_t hdr_len, total_len;

	if (size < sizeof(struct ip_hdr))
		return;

	hdr_len = iphdr->hdr_len << 2;
	printf("hdr_len: %d\n", hdr_len);
	if (iphdr->version != 4) {
		fprintf(stderr, "not ipv4\n");
		return;
	}
	if(size < hdr_len) {
		fprintf(stderr, "packet length error\n");
		return;
	}
	if (checksum_s((uint16_t *)iphdr, hdr_len, 0) != 0) {
		fprintf(stderr, "checksum error\n");
		return;
	}
	
	struct ip_interface *ifs = get_ip_interface_from_ether_port(port);
	if (ntohl(iphdr->dest_addr) != ifs->addr) {//dest_addr
		if (iphdr->dest_addr != ifs->broadcast && iphdr->dest_addr != ip_broadcast) {
			return;
		}
	}
	
	data += hdr_len;
	size -= hdr_len;

	//total_len = ntohs(iphdr->total_len) - iphdr->hdr_len;
	//printf("total_len: %d\n", total_len);
	switch (iphdr->proto) {
		case IP_PROTO_ICMP:
		{
			printf("*** ip proto icmp ***\n");
			rx_icmp(mbuf, data, size, ntohl(iphdr->src_addr), ntohl(iphdr->dest_addr));
			break;
		}
		case IP_PROTO_TCP:
		{
			printf("*** ip proto tcp ***\n");
			break;
		}
		case IP_PROTO_UDP:
		{
			printf("*** ip proto udp ***\n");
			rx_udp(mbuf, data, size, ntohl(iphdr->src_addr), ntohl(iphdr->dest_addr), ifs);
			break;
		}
		default:
		{
			printf("*** no ip proto ***\n");
			break;
		}
	}
	return;
}


