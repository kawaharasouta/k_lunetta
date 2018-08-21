#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>

#include"include/ip.h"
#include"include/ethernet.h"

#define ROUTE_TABLE_SIZE 2;

uint32_t get_ip_addr(struct ether_port *port) {
	return 0x0a000005;
}

/*********************/

//struct route_node {
//	uint8_t used;
//	uint32_t addr;
//	uint32_t mask;
//	uint32_t next;
//}

struct ip {//one interface
	struct ether_port port;
	uint32_t addr;
	uint32_t mask;
	uint32_t network;
	uint32_t broadcast;
	//struct route_node route_table[ROUTE_TABLE_SIZE];
};

struct route_node {
	uint8_t used;
	struct ip interface;
	uint32_t addr;
	uint32_t mask;
	uint32_t next;
};

struct route_node route_table[ROUTE_TABLE_SIZE];

/**********************/

/*
void 
route_table_init() {
	struct route_node *route;
	struct route_node *fin = ip.route_table + ROUTE_TABLE_SIZE;

	for (route = ip.route_table; route != fin; route++) {
		route->used = 0;
		route->addr = 0;
		route->mask = 0;
		route->next = 0;
	}
	return;
}

int route_table_add(const uint32_t addr, const uint32_t mask, const uint32_t next) {
	struct route_node *route
	struct route_node *fin = ip.route_table +  ROUTE_TABLE_SIZE;

	for (route = ip.route_table; route != fin; route++) {
		if (!route->used) {
			route->used = 1;
			route->addr = addr;
			route->mask = mask;
			route->next = next;
			return 0;
		}
	}
	return -1;
}

int
ip_init(const uint32_t addr, const uint32_t mask, const uint32_t gateway) {

	ip.addr = addr;
	ip.mask = mask;
	ip.network = addr & mask;
}
*/
