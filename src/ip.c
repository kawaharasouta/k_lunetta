#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>

#include"include/ip.h"
#include"include/ethernet.h"

#define IP_INTERFACE_NUM 1
#define ROUTE_TABLE_SIZE 3

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

void
tx_ip_core(uint8_t proto, struct rte_mbuf *mbuf, uint32_t size, uint32_t dest, uint32_t nexthop, struct ether_port *port) {
	
}

void 
tx_ip(uint8_t proto, struct rte_mbuf *mbuf, uint32_t size, uint32_t dest) {
	uint32_t nexthop = 0;
	struct ip_interface *ifs;
	struct ip_interface *fin = interfaces + IP_INTERFACE_NUM; 

	if (dest == ip_broadcast) {
		//all port
		for (ifs = interfaces; ifs != fin; ifs++) {
			tx_ip_core(proto, mbuf, size, dest, 0, ifs->port);
		}
	}
	else {

	}
//	for (ifs = interfaces; ifs != fin; ifs++) {
//		if (dest != ifs->broadcast) {
//			
//		}
//	}
	//if (dest != )
}
