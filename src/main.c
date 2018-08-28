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

//ethernet.c ...
void 
hexdump(uint8_t *buf, int size);

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
	//ip_init(get_port_pointer(), 1);
	struct ip_init_info ip_info;
	ip_info.port = get_port_pointer();
	ip_info.addr = 0x0a000006;
	ip_info.mask = 0xffffff00;
	ip_init(&ip_info, 1, ip_info.port, 0x0a000001);
	ip_interfaces_dump();
	route_table_dump();

	printf("udp_init\n");
	udp_init();
	udp_cb_dump();
	printf("\n");
	printf("udp_soc\n");
	int soc = lunetta_udp_soc();
	udp_cb_dump();
	printf("\n");
	printf("udp_bind\n");
	lunetta_udp_bind(soc, /*ip_info.addr*/0, 80);
	udp_cb_dump();
	printf("\n");


	int ret;
	int rx_pop_num;

	struct ether_port *ether_port = get_port_pointer();
	rte_eal_remote_launch(launch_lcore_rx, (void *)ether_port, 1);
	sleep(5);
//	rte_eal_wait_lcore(1);
//	while (1) {
//		launch_lcore_rx(ether_port);
//	}
		uint32_t tpa_ip = 0x0a000003;
		//struct rte_mbuf *mbuf;
		//mbuf = rte_pktmbuf_alloc(mbuf_pool);
		//tx_ip(6, mbuf, 0, tpa_ip, ip_info.addr);
		uint8_t *buf[256];
	while (1) {
		sleep(3);
		memset(buf, 0, 256);
		size_t len = udp_recvfrom(soc, buf, 256, NULL, NULL);
		hexdump(buf, (int)len);
		//uint32_t tpa = 0x0300000a;

		//printf("***\n");
		//uint8_t *p = rte_pktmbuf_mtod(mbuf, uint8_t*);
		//tx_ether(ether_port, mbuf, 0, ETHERTYPE_IP, &tpa, NULL);
		//tx_ip(6, mbuf, 0, tpa_ip, ip_info.addr);
	}
	rte_eal_wait_lcore(1);

}
