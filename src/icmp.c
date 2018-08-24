#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<rte_mbuf.h>

#include"include/icmp.h"
#include"include/ip.h"
#include"include/pkt_io.h"

#define ICMP_TYPE_ECHO_REPLY 0
#define ICMP_TYPE_ECHO_REQUEST 8


void 
icmp_send_rep(uint8_t *data, uint32_t size, uint32_t src, uint32_t dest) {
	struct rte_mbuf *mbuf = rte_pktmbuf_alloc(mbuf_pool);
	struct icmp_hdr_echo *recv_data = data;
	uint8_t *p = rte_pktmbuf_mtod(mbuf, uint8_t*);
	struct icmp_hdr_echo *packet = (struct icmp_hdr_echo *)p;
	uint32_t cpy_size = size - sizeof(struct icmp_hdr_echo);

	//printf("=== icmp_send_rep ===\n");
	packet->head.type = ICMP_TYPE_ECHO_REPLY;
	packet->head.code = 0;
	packet->head.check = 0;
	packet->id = recv_data->id;
	packet->sequence = recv_data->sequence;
	if (cpy_size > 0) 
		memcpy(p + sizeof(struct icmp_hdr_echo), data + sizeof(struct icmp_hdr_echo), cpy_size);

	packet->head.check = checksum_s((uint16_t *)p, (uint16_t)size, 0);

	tx_ip(IP_PROTO_ICMP, mbuf, size, src, dest);
}

void 
rx_icmp(struct rte_mbuf *mbuf, uint8_t *data, uint32_t size, uint32_t src, uint32_t dest) {
	//printf("@@@ rx_icmp @@@\n");
	if (size < sizeof(struct icmp_hdr_head)) {
		return;
	}

	//printf("size: %d\n\n", size);

	struct icmp_hdr_head *icmphdr_head = (struct icmp_hdr_head *)data;

	switch (icmphdr_head->type) {
		case ICMP_TYPE_ECHO_REPLY:
		{
			printf("--- icmp echo reply ---\n");
			break;
		}
		case ICMP_TYPE_ECHO_REQUEST:
		{
			printf("-- icmp echo request ---\n");
			icmp_send_rep(data, size, src, dest);
			break;
		}
		default:
		{
			printf("--- icmp no type ---\n");
			break;
		}
	}
}
