#ifndef __IP_H_
#define __IP_H_

#define IP_ADDR_LEN 4

#include<stdint.h>

extern struct ether_port;

struct ip_hdr{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	unsigned int hdr_len:4;
	unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
	unsigned int version:4;
	unsigned int hdr_len:4;
#else
# error "Please fix <bits/endian.h>"
#endif
	uint8_t type_of_service;
	uint16_t total_len;
	uint16_t id;
	uint16_t frag;
	uint8_t ttl;
	uint8_t proto;
	uint16_t check;
	uint32_t src_addr;
	uint32_t dest_addr;
};

struct ip_init_info {
	struct ether_port *port;
	uint32_t addr;
	uint32_t mask;
};


void print_ip_hdr(struct ip_hdr *ip_hdr);
void rx_ip(uint8_t *packet, uint32_t size, struct port_config *port);

uint32_t get_ip_addr(struct ether_port *port);

#endif
