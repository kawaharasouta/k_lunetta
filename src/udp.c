#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<pthread.h>
#include<arpa/inet.h>

#include<rte_mbuf.h>

#include"include/udp.h"
#include"include/ip.h"
#include"include/queue.h"
#include"include/pkt_io.h"

#define UDP_TABLE_NUM 16
#define UDP_SOURCE_PORT_MIN 49152
#define UDP_SOURCE_PORT_MAX 65535

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
	pthread_cond_t cond;
};

struct udp_table_info {
	struct udp_table_entry table[UDP_TABLE_NUM];
	pthread_mutex_t mutex;
} udp;

void udp_cb_dump() {
	struct udp_table_entry *entry;
	struct udp_table_entry *fin = udp.table + UDP_TABLE_NUM;
	printf("=== cb_dumo ===\n");
	pthread_mutex_lock(&udp.mutex);
	for (entry = udp.table; entry != fin; entry++) {
		printf("---------------\n");
		printf("used: %d\n", entry->used);
		printf("ifs: %p(pointer)\n", entry->ifs);
		printf("port: %u\n", entry->port);
		printf("queue: ***\n");
	}
	printf("===============\n");
	pthread_mutex_unlock(&udp.mutex);
}

int 
lunetta_udp_soc() {
	struct udp_table_entry *entry;
	struct udp_table_entry *fin = udp.table + UDP_TABLE_NUM;
	int index = 0;
	printf("udp_soc top\n");
	pthread_mutex_lock(&udp.mutex);
	printf("get mutex\n");
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
	if (addr != 0) {
		ifs = get_ip_interface_from_addr(addr);
		if (!ifs) {
			return -1;
		}
	}
	else {
		ifs = NULL;
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
tx_udp(uint16_t src_port, uint16_t dest_port, uint8_t *data, uint32_t size, uint32_t dest_ip, struct ip_interface *ifs) {
	struct rte_mbuf *mbuf = rte_pktmbuf_alloc(mbuf_pool);
	uint8_t *p = rte_pktmbuf_mtod(mbuf, uint8_t*);	
	struct udp_hdr *udphdr = (struct udp_hdr *)p;
	p += sizeof(struct udp_hdr);
	uint32_t udp_len = size + sizeof(struct udp_hdr);

	udphdr->src_port = htons(src_port);
	udphdr->dest_port = htons(dest_port);
	udphdr->len = htons(udp_len);
	udphdr->check = 0;
	memcpy(p, data, size);

	/* set checksum */
	//atodeyarou
	

	tx_ip(IP_PROTO_UDP, mbuf, udp_len, dest_ip, ifs);
	return;
}

void 
rx_udp(struct rte_mbuf *mbuf, uint8_t *data, uint32_t size, uint32_t src, uint32_t dest, struct ip_interface *ifs) {
	struct udp_hdr *udphdr = (struct udp_hdr *)data;
	void *queue_data;
	struct udp_queue_hdr *queue_hdr;

	printf("*** rx_udp ***\n");
	printf("ip_interface: %p(poiner)\n", ifs);

	//check size and checksum
	/* atodeyaru */

	struct udp_table_entry *entry;
	struct udp_table_entry *fin = udp.table + UDP_TABLE_NUM;
	pthread_mutex_lock(&udp.mutex);
	printf("mutex_lock\n");
	for (entry = udp.table; entry != fin; entry++) {
		if (entry->used && (!entry->ifs || entry->ifs == ifs) && entry->port == ntohs(udphdr->dest_port)) {//Implement INADDR_ANY later  ->  done?
			printf("match\n");
			queue_data = malloc(sizeof(struct udp_queue_hdr) + (size - sizeof(struct udp_hdr)));
			if (!queue_data) {
				pthread_mutex_unlock(&udp.mutex);
				return;
			}
			queue_hdr = queue_data;
			queue_hdr->addr = src;
			queue_hdr->port = udphdr->src_port;
			queue_hdr->len = size - sizeof(struct udp_hdr);
			memcpy(queue_hdr + 1, udphdr + 1, size - sizeof(struct udp_hdr));

			printf("queue_push\n");
			queue_push(&entry->queue, queue_data, size - sizeof(struct udp_hdr));
			printf("cond_signal\n");
			pthread_cond_signal(&entry->cond);
			printf("mutex_unlock\n");
			pthread_mutex_unlock(&udp.mutex);
			return;
		}
		//queue_hdr = queue_data;
		//queue_hdr->addr = src;
		//queue_hdr->port = udphdr->src_port;
		//queue_hdr->len = size - sizeof(struct udp_hdr);
		//memcpy(queue_hdr + 1, udphdr + 1, size - sizeof(struct udp_hdr));

		//queue_push(&entry->queue, queue_data, size - sizeof(struct udp_hdr));
		//pthread_cond_signal(&entry->cond);

		//pthread_mutex_unlock(&udp.mutex);
		//return;
	}
	printf("no match and mutex_unlock\n");
	pthread_mutex_unlock(&udp.mutex);
}

size_t 
udp_sendto(int soc, uint8_t *buf, size_t size, uint32_t peer, uint16_t dest_port) {
	struct udp_table_entry *entry, *ent;
	struct udp_table_entry *fin = udp.table + UDP_TABLE_NUM;
	struct ip_interface *ifs;
	uint16_t src_port, sport;
	if (soc < 0 || soc >= UDP_TABLE_NUM || !udp.table[soc].used) {
		return -1;
	}
	entry = &udp.table[soc];
	pthread_mutex_lock(&udp.mutex);
	ifs = entry->ifs;

	if (!ifs) {
		//ifs = get_ip_interface_from_addr(peer);
		ifs = get_ip_interface_from_peer(peer);
		if (!ifs) {
			pthread_mutex_unlock(&udp.mutex);
			return -1;
		}
	}
	if (!entry->port) {
		for (sport = UDP_SOURCE_PORT_MIN; sport <= UDP_SOURCE_PORT_MAX; sport++) {
			for (ent = udp.table; ent != fin; ent++) {
				if (ent->port == sport && (!ent->ifs || ent->ifs == ifs)) {
					break;
				}
			}
			if (ent == fin) {
				entry->port = sport;
				break;
			}
		}
		if (!entry->port) {
			pthread_mutex_unlock(&udp.mutex);
			return -1;
		}
	}

	src_port = entry->port;
	pthread_mutex_unlock(&udp.mutex);
	tx_udp(src_port, dest_port, buf, size, peer, ifs);
	return size;
}

size_t 
udp_recvfrom(int soc, uint8_t *buf, size_t size, uint32_t *addr, uint16_t *port) {
	struct udp_table_entry *entry;
	if (soc < 0 || soc >= UDP_TABLE_NUM || !udp.table[soc].used) {
		return -1;
	}
	entry = &udp.table[soc];
	pthread_mutex_lock(&udp.mutex);

	struct queue_node *node;
	while ((node = queue_pop(&entry->queue)) == NULL) {
		pthread_cond_wait(&entry->cond, &udp.mutex);
	}
	pthread_mutex_unlock(&udp.mutex);

	struct udp_queue_hdr *queue_hdr = node->data;
	if (addr) *addr = queue_hdr->addr;
	if (port) *port = queue_hdr->port;
	size_t len = size > queue_hdr->len ? queue_hdr->len : size;
	memcpy(buf, queue_hdr + 1, len);
	free(queue_hdr);
	return len;
}

void udp_init() {
	struct udp_table_entry *entry;
	struct udp_table_entry *fin = udp.table + UDP_TABLE_NUM;
	for (entry = udp.table; entry != fin; entry++) {
		pthread_cond_init(&entry->cond, NULL);
		queue_init(&entry->queue);
	}
	pthread_mutex_init(&udp.mutex, NULL);
	return;
}
