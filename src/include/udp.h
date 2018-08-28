#ifndef __UDP_H_
#define __UDP_H_

struct udp_hdr {
	uint16_t src_port;
	uint16_t dest_port;
	uint16_t len;
	uint16_t check;
};

#endif
