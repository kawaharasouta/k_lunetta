#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<pthread.h>

#include"include/udp.h"
#include"include/ip.h"
#include"include/queue.h"

#define UDP_TABLE_NUM 16

struct udp_queue_hdr {
	uint32_t addr;
	uint16_t port;
	uint16_t len;
	uint8_t data[0];
};

struct udp_table_entry {
	int used;
	struct ip_interface *ifs;
	uint16_t port;
	struct queue_info queue;
};

struct udp_table_info {
	struct udp_table_entry table[UDP_TABLE_NUM];
	pthread_mutex_t mutex;
} udp;

int 
lunetta_udp_soc() {
	struct udp_table_entry *entry;
	struct udp_table_entry *fin = udp.table + UDP_TABLE_NUM;
	int index = 0;
	pthread_mutex_lock(&udp.mutex);
	for (entry = udp.table; entry != fin; entry++, index++) {
		if (!entry->used) {
			entry->used = 1;
			pthread_mutex_unlock(&udp.mutex);
			return index;
		}
	}
	pthread_mutex_unlock(&udp.mutex);
	return -1;
}

void 
lunetta_udp_close() {

}

int 
lunetta_udp_bind(int soc, uint32_t addr, uint16_t port) {
	struct udp_table_entry *entry;
	struct ip_interface *ifs;

	if (soc < 0 || soc >= UDP_TABLE_NUM || !udp.table[soc].used) {
		return -1;
	}
	entry = &udp.table[soc];
	if (addr) {
		ifs = get_ip_interface_from_addr(addr);
		if (!ifs) {
			return -1;
		}
	}

	struct udp_table_entry *ent;
	struct udp_table_entry *fin = udp.table + UDP_TABLE_NUM;
	pthread_mutex_lock(&udp.mutex);
	for (ent = udp.table; ent != fin; ent++) {//If a match already exists
		if(ent->used && ent->ifs == ifs && ent->port == port) {
			pthread_mutex_unlock(&udp.mutex);
			return -1;
		}
	}
	entry->ifs = ifs;
	entry->port = port;
	pthread_mutex_unlock(&udp.mutex);
	return 0;
}

void 
rx_udp(struct rte_mbuf *mbuf, uint8_t *data, uint32_t size, uint32_t src, uint32_t dest, struct ip_interface *ifs) {
	struct udp_hdr *udphdr = (struct udp_hdr *)data;
	void *queue_data;
	struct udp_queue_hdr *queue_hdr;

	//check size and checksum
	/* atodeyaru */


	struct udp_table_entry *entry;
	struct udp_table_entry *fin = udp.table + UDP_TABLE_NUM;
	pthread_mutex_lock(&udp.mutex);
	for (entry = udp.table; entry != fin; entry++) {
		if (entry->used && entry->ifs == ifs && entry->port == ntohs(udphdr->dest_port)) {
			queue_data = malloc(sizeof(struct udp_queue_hdr) + (size - sizeof(struct udp_hdr)));
			if (!queue_data) {
				pthread_mutex_unlock(&udp.mutex);
				return;
			}
		}
		queue_hdr = queue_data;
		queue_hdr->addr = src;
		queue_hdr->port = udphdr->src_port;
		queue_hdr->len = size - sizeof(struct udp_hdr);
		memcpy(queue_hdr + 1, udphdr + 1, size - sizeof(struct udp_hdr));

		queue_push(entry->queue, queue_data, size - sizeof(struct udp_hdr));

		pthread_mutex_unlock(&udp.mutex);
		return;
	}
	pthread_mutex_unlock(&udp.mutex);
}
