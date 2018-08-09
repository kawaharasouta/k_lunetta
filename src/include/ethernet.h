#ifndef __ETHERNET_H_
#define __ETHERNET_H_


#define ETHERTYPE_IP 0x0800
#define ETHERTYPE_ARP 0x0806
#define ETHERTYPE_RARP 0x8035

#define ETHER_ADDR_LEN 6

//#define ETHER_PORT_MAX_NUM 1

extern struct port_config;

typedef struct {
	uint8_t addr[ETHER_ADDR_LEN];
} __attribute__ ((packed)) ethernet_addr;

struct ether_port {
	uint16_t port_num;
	ethernet_addr mac_addr;
};
//struct ether_port ports[ETHER_PORT_MAX_NUM];

struct ethernet_hdr {
	ethernet_addr dest;
	ethernet_addr src;
	uint16_t type;
} __attribute__ ((packed));

extern ethernet_addr ether_broadcast;

struct ether_port*
get_port_pointer();

int 
ethernet_init(struct port_config *port, uint16_t num);
void 
print_mac_addr(ethernet_addr *addr);
int 
equal_mac_addr(ethernet_addr *addr1, ethernet_addr *addr2);
int 
is_ether_broadcast(ethernet_addr *addr);
void 
rx_ether(struct ether_port *port, struct rte_mbuf *mbuf/*, uint32_t size*/);
void 
tx_ether(struct rte_mbuf *mbuf, uint32_t size, struct ether_port *port, uint16_t type, const void *paddr, ethernet_addr *dest);
#endif
