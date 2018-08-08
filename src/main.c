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


int 
main(void) {
	if (dpdk_init() == -1) {
		fprintf(stderr, "dpdk_init error\n");
		exit(1);
	}

	struct port_config port;
	port.port_num = 0;
	port_init(&port);
	int ret;
	int rx_pop_num;

	while (1) {
		rx_pkt(&port);

		struct rte_mbuf *mbuf;
		mbuf = rte_pktmbuf_alloc(mbuf_pool);
		uint8_t *p = rte_pktmbuf_mtod(mbuf, uint8_t*);
		struct ether_port *ether_port = get_port_pointer();
		//memset(p, 0x11, 60);
		//mbuf->pkt_len = 60;
		//mbuf->data_len = 60;
		//tx_pkt(&port, mbuf);
		tx_ether(mbuf, 0, /*&port*/ether_port, ETHERTYPE_IP, NULL, &ether_broadcast);
	}
}
