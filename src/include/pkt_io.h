#ifndef __PKT_IO_H_
#define __PKT_IO_H_

#define BURST_SIZE 32
#define RX_RING_SIZE 128
#define TX_RING_SIZE 512
#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

extern struct rte_mempool *mbuf_pool;

void tx_pkt (struct port_config *port, struct rte_mbuf *mbuf);
#endif
