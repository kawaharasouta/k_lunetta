#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<stdint.h>

#include"include/lunetta.h"
#include"include/pkt_io.h"
#include"include/ethernet.h"
#include"include/arp.h"
#include"include/ip.h"
#include"include/udp.h"

int 
lunetta_init(struct port_config *port_conf, struct ip_init_info *ip_info) {
  struct ether_port *ether_port;
	if (dpdk_init() == -1) {
		fprintf(stderr, "dpdk_init error");
		return -1;
  }

	port_init(port_conf);
	ethernet_init(port_conf, 1);
	arp_init(port_conf);
	ip_info->port = get_port_pointer();
	ip_init(ip_info, 1, ip_info->port, 0x0a000001);
	udp_init();

	ether_port = get_port_pointer();
	rte_eal_remote_launch(launch_lcore_rx, (void *)ether_port, 1);
	printf("launch application...\n");
	sleep(5);

  return 0;
}

int 
lunetta_close() {
	rte_eal_wait_lcore(1);
	return 0;
}

uint16_t 
checksum_s(uint16_t *data, uint16_t size, uint32_t init) {
  uint32_t sum;

  sum = init;
  while(size > 1) {
		sum += *(data++);
		size -= 2;
	}
	if(size) {
		sum += *(uint8_t *)data;
	}
	sum  = (sum & 0xffff) + (sum >> 16);
	sum  = (sum & 0xffff) + (sum >> 16);
	return ~(uint16_t)sum;
}
