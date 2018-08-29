#ifndef __UDP_H_
#define __UDP_H_

#include<rte_mbuf.h>

extern struct ip_interface;

struct udp_hdr {
	uint16_t src_port;
	uint16_t dest_port;
	uint16_t len;
	uint16_t check;
};

void udp_cb_dump();

int 
lunetta_udp_soc();
void 
lunetta_udp_close();
int 
lunetta_udp_bind(int soc, uint32_t addr, uint16_t port);
size_t 
udp_sendto(int soc, uint8_t *buf, size_t size, uint32_t peer, uint16_t dest_port);
size_t 
udp_recvfrom(int soc, uint8_t *buf, size_t size, uint32_t *addr, uint16_t *port);

struct ip_interface*
get_ip_interface_from_peer(uint32_t addr);

void 
tx_udp(uint16_t src_port, uint16_t dest_port, uint8_t *data, uint32_t size, uint32_t dest_ip, struct ip_interface *ifs);
void 
rx_udp(struct rte_mbuf *mbuf, uint8_t *data, uint32_t size, uint32_t src, uint32_t dest, struct ip_interface *ifs);
void 
udp_init();

#endif
