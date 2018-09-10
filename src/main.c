#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<rte_eal.h>
#include <inttypes.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_hexdump.h>
#include <rte_ether.h>

#include"include/lunetta.h"
#include"include/pkt_io.h"
#include"include/ethernet.h"
#include"include/arp.h"
#include"include/ip.h"
#include"include/udp.h"

void 
hexdump(uint8_t *buf, int size);

struct port_config port = {
	.port_num = 0
};
struct ip_init_info ip_info = {
	.addr = 0x0a000006,
	.mask = 0xffffff00
};

int 
main(void) {
	/* lunetta_init */
	if (lunetta_init(&port, &ip_info) == -1) {
		fprintf(stderr, "lunetta_init error\n");
		exit(1);
	}

	/* udp soc */
	printf("udp_soc\n");
	int soc = lunetta_udp_soc();
	printf("udp_bind\n");
	lunetta_udp_bind(soc, /*ip_info.addr*/0, 7);

	uint32_t tpa_ip;
	uint16_t tport; 
	uint8_t buf[UDP_DATA_MAX_LEN];
	size_t len;
	while (1) {
		printf("recv\n");
		len = udp_recvfrom(soc, buf, sizeof(buf), &tpa_ip, &tport);
		hexdump(buf, (int)len);
		printf("send\n");
		udp_sendto(soc, buf, len, tpa_ip, tport);
	}

	if (lunetta_close() == -1) {
		fprintf(stderr, "lunetta_close error\n");
		exit(1);
	}
	return 0;
}
