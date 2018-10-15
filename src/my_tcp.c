#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<pthread.h>
#include<arpa/inet.h>

#include<rte_mbuf.h>


#include"include/tcp.h"
#include"include/ip.h"
#include"include/queue.h"
#include"include/pkt_io.h"


typedef enum enm1 {
  CLOSED,
  SYN_SENT,
	ESTABLISHED,
	FIN_WAIT_1,
  FIN_WAIT_2,
	TIME_WAIT,
	CLOSE
} client_state;

typedef enum enm2 {
	CLOSED,
	LISTEN,
	SYN_RCVD,
	ESTABLISHED,
	CLOSE_WAIT,
	LAST_ACK,
} server_state;


