#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<stdint.h>

#include"include/lunetta.h"
#include"include/pkt_io.h"

int lunetta_init(struct port_config *conf) {
	if (dpdk_init() == -1) {
		fprintf(stderr, "dpdk_init error");
		return -1;
	}

	return 0;
}


