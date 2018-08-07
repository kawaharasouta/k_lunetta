#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include <rte_eal.h>
#include <inttypes.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_hexdump.h>
#include <rte_ether.h>

#include"include/lunetta.h"
#include"include/pkt_io.h"

struct rte_mempool *mbuf_pool;
static const struct rte_eth_conf port_conf_default = {
	.rxmode = { .max_rx_pkt_len = ETHER_MAX_LEN }
};

/*
 * Initializes a given port using global settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter.
 */
/*static */int
port_init(/*uint16_t port*/struct port_config *port)
{ 
	struct rte_eth_conf port_conf = port_conf_default;
	const uint16_t rx_rings = 1, tx_rings = 1;
	uint16_t nb_rxd = RX_RING_SIZE;
	uint16_t nb_txd = TX_RING_SIZE;
	int retval;
	uint16_t q;
	struct ether_addr addr;
	uint16_t nport = port->port_num;
	
	if (nport >= rte_eth_dev_count()) {
		return -1;
	}
	
	/* Configure the Ethernet port. */ 
	retval = rte_eth_dev_configure(nport, rx_rings, tx_rings, &port_conf);
	if (retval != 0) {
		return retval;
	}
	
	retval = rte_eth_dev_adjust_nb_rx_tx_desc(nport, &nb_rxd, &nb_txd);
	if (retval != 0) {
		return retval;
	}
	
	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (q = 0; q < rx_rings; q++) {
		retval = rte_eth_rx_queue_setup(nport, q, nb_rxd, rte_eth_dev_socket_id(nport), NULL, mbuf_pool);
		if (retval < 0) {
			return retval;
		}
	}
	
	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (q = 0; q < tx_rings; q++) {
		retval = rte_eth_tx_queue_setup(nport, q, nb_txd, rte_eth_dev_socket_id(nport), NULL);
		if (retval < 0) {
			return retval;
		}
	}
	
	/* Start the Ethernet port. */
	retval = rte_eth_dev_start(nport);
	if (retval < 0) {
		return retval;
	}
  
	/* Display the port MAC address. */
	rte_eth_macaddr_get(nport, &addr);
	printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
		nport,
		addr.addr_bytes[0], addr.addr_bytes[1],
		addr.addr_bytes[2], addr.addr_bytes[3],
		addr.addr_bytes[4], addr.addr_bytes[5]);

	for (int i = 0; i < ETHER_ADDR_LEN; i++) {
		port->mac_addr.addr[i] = addr.addr_bytes[i];
	}
	
	/* Enable RX in promiscuous mode for the Ethernet port. */
	rte_eth_promiscuous_enable(nport);
  
	return 0;
}

int
dpdk_init(void){
	int ret;
	unsigned nb_ports;
	uint16_t portid;  
	char *pg_name[] = {"k_lunetta"};

	ret = rte_eal_init(1, pg_name);
	if (ret < 0) {
		return -1;
	}
  
	nb_ports = rte_eth_dev_count();
	if (nb_ports != 1) {
		return -1;
	}
	
	/* Creates a new mempool in memory to hold the mbufs. */
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
  
	if (mbuf_pool == NULL) {
		return -1;
	}
  
	return 0;
}


void
rx_pkt (struct port_config *port) {
	uint16_t nport = port->port_num;
	struct rte_mbuf *bufs[BURST_SIZE];
	uint16_t nb_rx = 0;

	/* Recv burst of RX packets */
	nb_rx = rte_eth_rx_burst(nport, 0, bufs, BURST_SIZE);
	for (int i = 0; i < nb_rx ; i++) {
		//uint8_t *p = rte_pktmbuf_mtod(bufs[i], uint8_t*);
		uint32_t size = rte_pktmbuf_pkt_len(bufs[i]);
		//rte_hexdump(stdout, "", (const void *)p, size);
#ifndef DEBUG_PKT_IO
		rx_ether(port, bufs[i], size);
#endif
		/* Tentatively */
		rte_pktmbuf_free(bufs[i]);
	}
}
void
tx_pkt (struct port_config *port, struct rte_mbuf *mbuf) {
	struct rte_mbuf *bufs[BURST_SIZE];
	uint16_t nport = port->port_num;
	int i, j, k;
	uint16_t nb_tx;

	mbuf->port = port->port_num;
	mbuf->packet_type = 1;
	bufs[0] = mbuf;
	nb_tx = rte_eth_tx_burst(nport, 0, bufs, 1);
	if (nb_tx == 0)
		rte_pktmbuf_free(bufs[0]);
	return;
}


/** 1lcore per 1port **/
void lcore_rxtxmain(struct port_config *port) {
	printf("lcore_rxtxmain");
	printf("port->port_num: %u\n", port->port_num);
	while (1) {
		rx_pkt(port);
		struct rte_mbuf *mbuf;
		mbuf = rte_pktmbuf_alloc(mbuf_pool);
		uint8_t *p = rte_pktmbuf_mtod(mbuf, uint8_t*);
		memset(p, 1, 60);
		mbuf->pkt_len = 60;
		mbuf->data_len = 60;
		tx_pkt(port, mbuf);
	}
}
int launch_lcore_rxtx(void *arg) {
	unsigned lcore_id = rte_lcore_id();
	printf("lcore%u launched\n", lcore_id);

	lcore_rxtxmain((struct port_config *)arg);
	return 0;
}



#ifdef DEBUG_PKT_IO
int main(void) {
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
		memset(p, 0x11, 60);
		mbuf->pkt_len = 60;
		mbuf->data_len = 60;
		tx_pkt(&port, mbuf);
	}
}
#endif
