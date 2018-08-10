#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>

#include<rte_mbuf.h>

#include"include/arp.h"
#include"include/ip.h"
#include"include/ethernet.h"
#include"include/pkt_io.h"

#define ARP_TABLE_SIZE 4096

struct arp_entry {
	uint32_t pa;
	ethernet_addr ha;
	time_t timestamp;
	void *data;
	size_t size;
	struct ether_port *port;
	struct arp_entry *next;
};

static struct {
	time_t timestamp;
	struct {
		struct arp_entry table[ARP_TABLE_SIZE];
		struct arp_entry *head;
		struct arp_entry *pool;
	} table;
	pthread_mutex_t mutex;
	//struct port_config *port;
} arp_table;

void arp_table_dump() {
	struct arp_entry *entry;
	printf("===========arp_table_dump==========\n");
	for (entry = arp_table.table.head; entry; entry = entry->next) {
		printf("----------------------------------\n");
		printf("ip addr: %x\tmac addr: ", entry->pa);
		print_mac_addr(&entry->ha);
	}
	printf("=============end table==============\n");

	return;
}

void arp_init(struct port_config *port) {
	int i;
	struct arp_entry *entry;
	
	time(&arp_table.timestamp);
	for (i = 0; i < ARP_TABLE_SIZE; i++) {
		entry = arp_table.table.table + i;
		entry->pa = 0;
		entry->timestamp = 0;
		memset(&entry->ha, 0, sizeof(ethernet_addr));
		entry->data = NULL;
		entry->size = 0;
		entry->port = NULL;
		entry->next = (i != ARP_TABLE_SIZE) ? (entry + 1) : NULL;
	}
	arp_table.table.head = NULL;
	arp_table.table.pool = arp_table.table.table;

	pthread_mutex_init(&arp_table.mutex, NULL);
	return 0;
}

int arp_table_select(const uint32_t *pa, ethernet_addr *ha) {
	struct arp_entry *entry;
	for (entry = arp_table.table.head; entry; entry = entry->next) {
		if (!entry) printf("!entry\n");
		if (entry->pa == *pa) {
			memcpy(ha, &entry->ha, sizeof(ethernet_addr));
			return 0;
		}
	}
	return -1;
}

int arp_table_renew(const uint32_t *pa, const ethernet_addr *ha, struct ether_port *port) {
	struct arp_entry *entry;

	for (entry = arp_table.table.head; entry; entry = entry->next) {
		if (entry->pa == *pa) {
			memcpy(&entry->ha, ha, sizeof(ethernet_addr));
			entry->port = port;
			time(&entry->timestamp);
			//pthread_cond_broadcast(&entry->cond);
			if (entry->data) {
				//ethernet_output(ETHERNET_TYPE_IP, (uint8_t *)entry->data, entry->len, NULL, &entry->ha);
				struct rte_mbuf *mbuf;
				mbuf = rte_pktmbuf_alloc(mbuf_pool);
				uint8_t *p = rte_pktmbuf_mtod(mbuf, uint8_t*);
				rte_memcpy(p, entry->data, entry->size);
			
				tx_ether(port, mbuf, entry->size, ETHERTYPE_IP, pa, ha);
				//free(entry->data);
				entry->data = NULL;
				entry->size = 0;
			}
			return 1;
		}
	}
	return 0;
}

int arp_table_insert(const uint32_t *pa, const ethernet_addr *ha, struct ether_port *port) {
	struct arp_entry *entry;
	printf("arp_table_insert\n");
	printf("t_ip_addr: %x\n", *pa);

	entry = arp_table.table.pool;
	if (!entry) {
			return -1;
	}
	entry->pa = *pa;
	memcpy(&entry->ha, ha, sizeof(ethernet_addr));
	entry->port =port;
	time(&entry->timestamp);
	arp_table.table.pool = entry->next;
	entry->next = arp_table.table.head;
	arp_table.table.head = entry;
	return 0;
}

int arp_resolve(struct ether_port *port, const uint32_t *pa, ethernet_addr *ha, const void *data, uint32_t size) {
	struct arp_entry *entry;
	
	pthread_mutex_lock(&arp_table.mutex);
	if (arp_table_select(pa, ha) == 0) {
			pthread_mutex_unlock(&arp_table.mutex);
			return 1;
	}

	/* If it does not exist in arp_table, save the data in the table and send arp_req. */
	if (!data) {
			pthread_mutex_unlock(&arp_table.mutex);
			return -1;
	}
	entry = arp_table.table.pool;
	if (!entry) {
			pthread_mutex_unlock(&arp_table.mutex);
			return -1;
	}
	entry->data = malloc(size);
	if (!entry->data) {
			pthread_mutex_unlock(&arp_table.mutex);
			return -1;
	}
	memcpy(entry->data, data, size);
	entry->port = port;
	entry->size = size;
	arp_table.table.pool = entry->next;
	entry->next = arp_table.table.head;
	arp_table.table.head = entry;
	entry->pa = *pa;
	time(&entry->timestamp);
	send_req(port, pa);
	pthread_mutex_unlock(&arp_table.mutex);
	return	0;
}

void send_req(struct ether_port *port, const uint32_t *tpa) {
	struct rte_mbuf *mbuf;
	mbuf = rte_pktmbuf_alloc(mbuf_pool);

	struct arp_ether *request;
	uint8_t *p = rte_pktmbuf_mtod(mbuf, uint8_t*);
	request = (struct arp_ether *)p;

	if (!tpa) {
		return;// -1;
	}
	request->arphdr.hrd_type = htons(ARP_HRD_ETHERNET);
	request->arphdr.proto_type = htons(ETHERTYPE_IP);
	request->arphdr.hrd_len = 6;
	request->arphdr.proto_len = 4;
	request->arphdr.ar_op = htons(ARPOP_REQUEST);
	for(int i = 0; i < ETHER_ADDR_LEN; i++) {
		request->s_eth_addr.addr[i] = port->mac_addr.addr[i];
	}
	request->s_ip_addr = htonl(get_ip_addr(port));
	memset(&request->d_eth_addr, 0, ETHER_ADDR_LEN);
	request->d_ip_addr = *tpa;
	//request->d_ip_addr = htonl(0x0a000003);


	tx_ether(port, mbuf, sizeof(struct arp_ether), ETHERTYPE_ARP, NULL, &ether_broadcast);
	return;//  0;
}

void send_rep(struct ether_port *port, const uint32_t *tpa, const ethernet_addr *tha) {
	struct rte_mbuf *mbuf;
	mbuf = rte_pktmbuf_alloc(mbuf_pool);

	struct arp_ether *rep;
	uint8_t *p = rte_pktmbuf_mtod(mbuf, uint8_t*);
	rep = (struct arp_ether *)p;

	if (!tpa || !tha) {
		return;// -1;
	}
	rep->arphdr.hrd_type = htons(ARP_HRD_ETHERNET);
	rep->arphdr.proto_type = htons(ETHERTYPE_IP);
	rep->arphdr.hrd_len = 6;
	rep->arphdr.proto_len = 4;
	rep->arphdr.ar_op = htons(ARPOP_REPLY);
	for (int i = 0; i < ETHER_ADDR_LEN; i++) {
		rep->s_eth_addr.addr[i] = port->mac_addr.addr[i];
	}
	rep->s_ip_addr = /* htonl(port->ip_addr); */htonl(get_ip_addr(port));
	for (int i = 0; i < ETHER_ADDR_LEN; i++) {
		rep->d_eth_addr.addr[i] = tha->addr[i];
	}
	rep->d_ip_addr = htonl(*tpa);

	tx_ether(port, mbuf, sizeof(struct arp_ether), ETHERTYPE_ARP, tpa, NULL);
}

void rx_arp(struct ether_port *port, struct rte_mbuf *mbuf, uint8_t *data, uint32_t size) {
	struct arp_ether *hdr;
	int merge_flag = 0;
	
	printf("rx_arp\n");

	if (size < sizeof(struct arp_ether)) 
		return;

	hdr = (struct arp_ether *)data;
	if (ntohs(hdr->arphdr.hrd_type) != ARP_HRD_ETHERNET) return;
	if (ntohs(hdr->arphdr.proto_type) != ETHERTYPE_IP) return;
	if (hdr->arphdr.hrd_len != ETHER_ADDR_LEN) return;
	if (hdr->arphdr.proto_len != IP_ADDR_LEN) return;
	pthread_mutex_lock(&arp_table.mutex);
	merge_flag = arp_table_renew(&hdr->s_ip_addr, &hdr->s_eth_addr, port);
	pthread_mutex_unlock(&arp_table.mutex);

	uint32_t addr = get_ip_addr(port);
	printf("s_ip_addr: %x\n", ntohl(hdr->s_ip_addr));

	if (ntohl(hdr->d_ip_addr) == addr) {
		if (merge_flag == 0) {
			pthread_mutex_lock(&arp_table.mutex);
			arp_table_insert(&hdr->s_ip_addr, &hdr->s_eth_addr, port);
			pthread_mutex_unlock(&arp_table.mutex);
		}
		if (ntohs(hdr->arphdr.ar_op) == ARPOP_REQUEST)
			/* port?	addr? */
			send_rep(/*&hdr->s_ip_addr*/port, &hdr->s_ip_addr, &hdr->s_eth_addr);
	}

	arp_table_dump();

	return;
}
