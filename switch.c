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



void switch_main(int switch_id) {

	struct net_port *node_port_list;
	struct net_port **node_port;
	int node_port_num;
	struct packet *in_packet;
	struct packet *new_packet;
	struct net_port *p;
	struct host_job *new_job;
	struct job_queue job_q;
	int i, j, k, n;
	int entry_id;

	/*
	 * Create array node_port[] that stores the network link ports
	 * at the switch.  The number of ports on switch is node_port_num.
	 */
	node_port_list = net_get_port_list(switch_id);

	/* Counts number of network link ports */
	node_port_num = 0;
	for (p = node_port_list; p != NULL; p=p->next) {
		node_port_num++;
	}

	/* Create memory space for array */
	node_port = (struct net_port **) malloc(node_port_num*sizeof(struct net_port *));
	
	/* Load ports into array */
	p = node_port_list;
	for (k = 0; k < node_port_num; k++) {
		node_port[k] = p;
		p = p->next;
	}
	
	/* Initialize Job Queue */
	job_q_init(&job_q);
	
	/* Initialize Forwarding Table */
	switch_forwarding_table forwarding_table[node_port_num];
	for (i = 0; i < node_port_num; i++) {
		forwarding_table[i].valid = 0;
		forwarding_table[i].dst_switch_id = 0;
		forwarding_table[i].port = 0;
	}

	while(1) {
	/*
	 * Get packets from incoming ports and translate to jobs.
	 * Put jobs in job queue
	 */
		/* Scan all Ports */
		for (i = 0; i < node_port_num; i++) { 
			in_packet = (struct packet *) malloc(sizeof(struct packet));
			new_job = (struct host_job *) malloc(sizeof(struct host_job));
			n = packet_recv(node_port[i], in_packet);	
			
			/* If switch receives packet, create new job */
			if (n > 0) {
				new_job->in_port_index = i;
				new_job->packet = in_packet;
				job_q_add(&job_q, new_job);
			}
			else {
				free(new_job);
				free(in_packet);
			}
		}
			/* Execute one job from queue */
			if (job_q_num(&job_q) > 0) {
				entry_id = -1;
				new_job = job_q_remove(&job_q);
				/*
				 * Check if entry exists in forwarding table 
				 */
					entry_id = containsEntry(forwarding_table, new_job->packet->dst, node_port_num);
					if (entry_id != -1) {
						packet_send(node_port[forwarding_table[entry_id].port],new_job->packet);
					}
					/* Sends packet on all ports */
					else {
						for (j = 0; j < node_port_num; j++) {
							if (j != new_job->in_port_index)  {
								printf("Sending on port %d...\n", j);
								packet_send(node_port[j], new_job->packet);
							}
						}	
					}
				/*
				 * Adds source packet to forwarding table.
				 */
					if (entry_id == -1) {
						for (int k = 0; k < node_port_num; k++) {
							if (forwarding_table[k].valid == 0) {
								forwarding_table[k].valid = 1;
								forwarding_table[k].dst_switch_id = (int)new_job->packet->src;
								forwarding_table[k].port = (int)new_job->in_port_index; //node port number
								break;
							}
						}
					}
					printf("---------------------------------------\n");
					printf("Valid\t");
					printf("Destination (Host ID)\t");
					printf("Port #\t\n");
					for (n = 0; n < node_port_num; n++) {
						printf("%d\t%d\t\t\t%d\n", forwarding_table[n].valid, forwarding_table[n].dst_switch_id, forwarding_table[n].port);
					}
					printf("---------------------------------------\n");
			}
			/* Sleep for 10 ms */
			usleep(TENMILLISEC);
	
	
	} /* End of while */

} /* end of main */

int containsEntry(switch_forwarding_table forwarding_table[], char dst, int number_of_entries) {
	for (int i = 0; i < number_of_entries; i++) {
		if((forwarding_table[i].valid == 1) && (forwarding_table[i].dst_switch_id == dst)) {
			return i;
		}
		printf("Could not find entry in table\n");
		return -1;
	}
}

