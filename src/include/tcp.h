#ifndef __TCP_H_
#define __TCP_H_

struct tcp_hdr {
	uint16_t src_port;
	uint16_t dest_port;
	uint32_t seqence;
	uint32_t acknowledge;
	uint8_t offset;
	uint8_t flag;
	uint16_t window_size;
	uint16_t checksum;
	uint16_t urg;
};

#endif 
