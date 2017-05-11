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
#include "host.h"
#include "packet.h"
#include "switch.h"
#include "dns.h"

#define TENMILLISEC 10000   /* 10 millisecond sleep */


void job_q_init(struct job_queue *j_q);
void job_q_add(struct job_queue *j_q, struct host_job *j);
int job_q_num(struct job_queue *j_q);

void switch_main(int host_id) {

struct host_job *job_q_remove(struct job_queue *j_q);

struct net_port *node_port_list;
struct net_port **node_port;
int node_port_num;

struct packet *in_packet;
struct packet *new_packet;

struct net_port *p;
struct host_job *new_job, *new_job1;

struct job_queue job_q;
int n;
char new_entry, entry_exists;

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
}

/* Initialize Job Queue */
job_q_init(&job_q);

/* Initialize Forwarding Table */
struct forwarding_table_entry forwarding_table[200];
for (int i = 0; i < 200; i++) {
	forwarding_table[i].valid = 0;
	forwarding_table[i].dst_host_id = 0;
	forwarding_table[i].port = 0;
}
/*
 * Get packets from incoming ports and translate to jobs.
 * Put jobs in job queue
 */

while(1) {
	//Scans All Ports
	for (int i = 0; i < node_port_num; i++) { 
		in_packet = (struct packet *) malloc(sizeof(struct packet));
		n = packet_recv(node_port[i], in_packet);	
		
		//If switch receives packet, create new job
		if (n > 0) {
			new_job = (struct host_job *) malloc(sizeof(struct host_job));
			new_job->in_port_index = i;
			new_job->packet = in_packet;
			job_q_add(&job_q, new_job);
			free(new_job);
			free(in_packet);
		}
	}
		//Execute one job from queue
		if (job_q_num(&job_q) > 0) {
			new_entry = '1';
			entry_exists = '0';
			new_job1 = job_q_remove(&job_q);
			/*
			 * Check if entry exists in forwarding table 
			 */
				for (int j = 0; j < 200; j++) {
					if ((forwarding_table[j].valid == 1) && (new_job1->packet->dst == forwarding_table[j].dst_host_id )) {
						new_entry = '0';

						//Send packet to correct port
						packet_send(node_port[forwarding_table[j].port],new_job1->packet);
					}
					if ((forwarding_table[j].valid == 1) && (forwarding_table[j].dst_host_id == (int)new_job1->packet->src)) {
						entry_exists = '1';
					}
				}
			/*
			 * Adds source packet to forwarding table.
			 */
				for (int j = 0; j < 200; j++) {
					if ((forwarding_table[j].valid == 0) && (entry_exists == '0')) {
						forwarding_table[j].valid = 1;
						forwarding_table[j].dst_host_id = (int)new_job1->packet->src;
						forwarding_table[j].port = (int)new_job1->in_port_index; //node port number
						break;
					}
				}
				if (new_entry == '1') {
					/* Sends packet on all ports except source port
					 * Should it send only on unknown ports?
					 **/
					for (int j = 0; j < 200; j++) {
						if (j != new_job1->in_port_index)  {
							printf("Sending on port %d...\n", j);
							packet_send(node_port[j], new_job1->packet);
						}
					}	
				}
				//printf("---------------------------------------\n");
				//printf("Valid\t");
				//printf("Destination (Host ID)\t");
				//printf("Port #\t\n");
				//for (int j = 0; j < node_port_num; j++) {
				//	printf("%d\t%d\t\t\t%d\n", forwarding_table[j].valid, forwarding_table[j].dst_host_id, forwarding_table[j].port);
				//}
				//printf("---------------------------------------\n");
		}
		/* Sleep for 10 ms */
		usleep(TENMILLISEC);


} /* End of while */

} /* end of main */



