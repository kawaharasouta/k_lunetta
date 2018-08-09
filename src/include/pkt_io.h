#ifndef __PKT_IO_H_
#define __PKT_IO_H_

#define BURST_SIZE 32
#define RX_RING_SIZE 128
#define TX_RING_SIZE 512
#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

extern struct rte_mempool *mbuf_pool;

int
launch_lcore_rx(void *arg);
void
tx_pkt(uint16_t port_num, struct rte_mbuf **mbuf, int num);
#endif
