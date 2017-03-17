/*
 * switch.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "net.h"
#include "man.h"
#include "host.h"
#include "packet.h"
#include "switch.h"

#define TENMILLISEC 10000   /* 10 millisecond sleep */


void switch_main(int host_id) {

struct net_port *node_port_list;
struct net_port **node_port;
int node_port_num;

struct packet *in_packet;
struct packet *new_packet;

struct net_port *p;
struct host_job *new_job;
struct host_job *new_job2;

struct job_queue job_q;

/*
 * Create array node_port[] that stores the network link ports
 * at the switch.  The number of ports on switch is node_port_num.
 */
node_port_list = net_get_port_list(host_id);

/* Counts number of network link ports */
node_port_num = 0;
for (p = node_port_list; p != NULL; p=p->next) {
	node_port_num++;
}

/* Create memory space for array */
node_port = (struct net_port **) malloc(node_port_num*sizeof(struct net_port *));

/* Load ports into array */
p = node_port_list;
for (int k = 0; k < node_port_num; k++) {
	node_port[k] = p;
	p = p->next;
	printf("%d\n", node_port[k]);
}


}
