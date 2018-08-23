#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<stdint.h>

#include"include/lunetta.h"
#include"include/pkt_io.h"

int 
lunetta_init(struct port_config *conf) {
  if (dpdk_init() == -1) {
	fprintf(stderr, "dpdk_init error");
	return -1;
  }

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
