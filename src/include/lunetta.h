#ifndef __LUNETTA_H_
#define __LUNETTA_H_

#include"ethernet.h"

struct port_config {
	uint16_t port_num;
	ethernet_addr mac_addr;
	uint32_t ip_addr;
	uint32_t ip_netmask;
	uint32_t ip_gateway;
};

int
lunetta_init();

uint16_t 
checksum_s(uint16_t *data, uint16_t size, uint32_t init);

#endif
