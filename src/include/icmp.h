#ifndef _ICMP_H_
#define _ICMP_H_

struct icmp_hdr_head {
	uint8_t type;
	uint8_t code;
	uint16_t check;	
};

struct icmp_hdr_echo {
	struct icmp_hdr_head head;
	uint16_t id;
	uint16_t sequence;
};

struct icmp_hdr_redirect {
	struct icmp_hdr_head head;
	uint32_t gateway_addr;
};


void
rx_icmp(struct rte_mbuf *mbuf, uint8_t *data, uint32_t size, uint32_t src, uint32_t dest);

#endif 
