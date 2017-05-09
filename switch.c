/*
 * switch.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include "main.h"
#include "net.h"
#include "man.h"
#include "host.h"
#include "packet.h"
#include "switch.h"

#define TENMILLISEC 10000   /* 10 millisecond sleep */
#define PORTMAX	100


void switch_main(int switch_id) {

	struct net_port *node_port_list;
	struct net_port **node_port;
	int node_port_num;
	struct packet *in_packet;
	struct packet *new_packet;
	struct packet *control_packet;
	struct net_port *p;
	struct host_job *new_job, *new_job2;
	struct job_queue job_q;
	int i, j, k, n;
	int entry_id;
	
	int localRootID, localRootDist, localParent;
	int control_counter = 0;	

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
	switch_forwarding_table forwarding_table[PORTMAX];
	for (i = 0; i < PORTMAX; i++) {
		forwarding_table[i].valid = 0;
		forwarding_table[i].dst_switch_id = 0;
		forwarding_table[i].port = 0;
	}

	/* Initialize tree Variables */
	char localPortTree[node_port_num];
	localRootID = switch_id;
	localRootDist = 0;
	localParent = -1;

	for (int i = 0; i < node_port_num; i++) {
		localPortTree[i] = 'N';
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
			/* If switch receives non control-packet, create new job */
			if ((n > 0) && (in_packet->type != PKT_CONTROL)) {
				printf("adding to job queue\n");
				new_job->in_port_index = i;
				new_job->packet = in_packet;
				job_q_add(&job_q, new_job);
			}
			/* If switch receives control packet, update variables */
			else if ((n > 0 ) && (in_packet->type == PKT_CONTROL)){
				/* Update localRootID, localRootDist, and localParent */
				int payloadRoot = in_packet->payload[0] - '0';
				int payloadDist = in_packet->payload[1] - '0';
				if (in_packet->payload[2] == 'S') {
					if(payloadRoot < localRootID) { // Found a better root
						localRootID = payloadRoot; // Node becomes the child of the neighbor at port k
						localParent = i;
						localRootDist = payloadDist + 1; // New distance = neighbor disance + one hop to neighbor
					}
					else if (payloadRoot == localRootID) { // The root is the same
						if (localRootDist > payloadDist+1) { // Found a better path to the root
							localParent = i;
							localRootDist = payloadDist + 1; // New distance = Neighbor dit + one hop to neighbor
						}
					}
				}
				/* Update status of local port, whether it is in the tree or not */
				if (in_packet->payload[2] == 'H') { // Port is attached to the parent so its part of tree
					localPortTree[i] = 'Y';
				}
				else if (in_packet->payload[2] == 'S') {
					if (localParent == i) {
						localPortTree[i] = 'Y';
					}
					else if (in_packet->payload[3] == 'Y') { // Port is attached to child, part of tree
						localPortTree[i] = 'Y';
					}
					else {
						localPortTree[i] = 'N';
					}
				}
				else {
					localPortTree[i] = 'N';
				}
//				printf("localPortTree[%d] = %c\n", i, localPortTree[i]);
			}
			else {
				control_packet = (struct packet *) malloc(sizeof(struct packet));
				if (control_counter >= 100) {
				/*
				 * Builds control packet
				 */
					for (k = 0; k < node_port_num; k++) {
						control_packet->src =  switch_id;
						control_packet->dst =  -1;
						control_packet->type = PKT_CONTROL; 
						control_packet->length = 4;
						control_packet->payload[0] = localRootID + '0';
						control_packet->payload[1] = localRootDist + '0';
						control_packet->payload[2] = 'S';
						if (i == localParent) {
							control_packet->payload[3] = 'Y'; 
						}
						else {
							control_packet->payload[3] = 'N';
						}
//						printf("Control Packet Payload: %s\n", control_packet->payload);
						packet_send(node_port[k], control_packet);
					}
					control_counter = 0;
				}
				free(new_job);
				free(in_packet);
			}
		} // end for loop	

		/* Execute one job from queue */
		if (job_q_num(&job_q) > 0) {
			entry_id = -1;
			new_job = job_q_remove(&job_q);
			in_packet = new_job->packet;
			printf("executing non control job\n");
			/*
			 * Check if entry exists in forwarding table 
			 */
				entry_id = containsEntry(forwarding_table, in_packet->src, PORTMAX);
				if (entry_id != -1) {
					packet_send(node_port[forwarding_table[entry_id].port],new_job->packet);
				}
				/* Sends packet on all ports */
				else {
					for (j = 0; j < node_port_num; j++) {
						printf("localPortTree[%d] = %c\n", j, localPortTree[j]);
						if ((j != new_job->in_port_index) && (localPortTree[j] == 'Y'))  {
							printf("Sending on port %d...\n", j);
							packet_send(node_port[j], new_job->packet);
						}
					}	
				}
			/*
			 * Adds source packet to forwarding table.
			 */
				if (entry_id == -1) {
					for (int k = 0; k < PORTMAX; k++) {
						if (forwarding_table[k].valid == 0) {
							forwarding_table[k].valid = 1;
							forwarding_table[k].dst_switch_id = new_job->packet->src;
							forwarding_table[k].port = new_job->in_port_index; //node port number
							break;
						}
					}
				}
				printf("---------------------------------------\n");
				printf("Valid\t");
				printf("Destination (Host ID)\t");
				printf("Port #\t\n");
				for (n = 0; n < PORTMAX; n++) {
					if(forwarding_table[n].valid) {
						printf("%d\t%d\t\t\t%d\n", forwarding_table[n].valid, forwarding_table[n].dst_switch_id, forwarding_table[n].port);
					}
				}
				printf("---------------------------------------\n");
			
		}
		free(control_packet);
		control_counter++;
		/* Sleep for 10 ms */
		usleep(TENMILLISEC);
	
	
	} /* End of while */

} /* end of main */

int containsEntry(switch_forwarding_table forwarding_table[], int dst, int number_of_entries) {
	for (int i = 0; i < number_of_entries; i++) {
		if((forwarding_table[i].valid == 1) && (forwarding_table[i].dst_switch_id == dst)) {
			return i;
		}
	}
	//printf("Could not find entry in table\n");
	return -1;
}

