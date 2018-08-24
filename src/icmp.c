#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<rte_mbuf.h>

#include"include/icmp.h"

#define ICMP_TYPE_ECHO_REPLY 0
#define ICMP_TYPE_ECHO_REQUEST 8

void 
icmp_send_rep(uint8_t *data, uint32_t size, uint32_t *src, uint32_t *dest) {
	//struct rte_mbuf *mbuf = 
	
}

void 
rx_icmp(struct rte_mbuf *mbuf, uint8_t *data, uint32_t size, uint32_t *src, uint32_t *dest) {
	if (size < sizeof(struct icmp_hdr_head));
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
			//send_rep();
			break;
		}
		default:
		{
			printf("--- icmp no type ---\n");
			break;
		}
	}
}
