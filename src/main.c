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


int 
main(void) {
	if (dpdk_init() == -1) {
		fprintf(stderr, "dpdk_init error\n");
		exit(1);
	}

	struct port_config port;
	port.port_num = 0;
	port_init(&port);
	ethernet_init(&port, 1);
	arp_init(&port);
	ip_interfaces_init(get_port_pointer(), 1);
	int ret;
	int rx_pop_num;

	struct ether_port *ether_port = get_port_pointer();
	rte_eal_remote_launch(launch_lcore_rx, (void *)ether_port, 1);
//	rte_eal_wait_lcore(1);
//	launch_lcore_rx(ether_port);
	while (1) {
		uint32_t tpa = 0x0300000a;

		//printf("***\n");
		//struct rte_mbuf *mbuf;
		//mbuf = rte_pktmbuf_alloc(mbuf_pool);
		//uint8_t *p = rte_pktmbuf_mtod(mbuf, uint8_t*);
		//tx_ether(ether_port, mbuf, 0, ETHERTYPE_IP, &tpa, NULL);
		sleep(3);
		send_req(ether_port, &tpa);
	}
	rte_eal_wait_lcore(1);

}
