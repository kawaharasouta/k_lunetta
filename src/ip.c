#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>

#include"include/ip.h"
#include"include/ethernet.h"

#define IP_INTERFACE_NUM 1
#define ROUTE_TABLE_SIZE 2//for instance

uint32_t get_ip_addr(struct ether_port *port) {
	return 0x0a000005;
}

struct ip_interface {//one interface
	struct ether_port *port;
	uint32_t addr;
	uint32_t mask;
	uint32_t network;
	uint32_t broadcast;
	//struct route_node route_table[ROUTE_TABLE_SIZE];
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


void 
ip_interfaces_init(struct ether_port /* * */*port, uint16_t num) {
	if (num > IP_INTERFACE_NUM) {
		fprintf(stderr, "ip_interfaces_init error\n");
		exit(1);
	}
	//now one loop
	for (int i = 0; i < num; i++) {
		interfaces[i].port = port;
		interfaces[i].addr = 0x0a000005;
		interfaces[i].mask = 0xffffff00;
		interfaces[i].network = 0x0a000000;
		interfaces[i].broadcast = 0x0a0000ff;
	}
	return;
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
	//ip_interfaces_init(port, num);
	//if (route_table_add(0x0a000005, 0xffffff00, 0x00000000, loopback ether_port))
	struct ip_interface *ifs = interfaces;

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

