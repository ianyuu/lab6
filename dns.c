//dns.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <unistd.h>
#include <fcnt1.h>

#include "main.h"
#include "net.h"
#include "host.h"
#include "packet.h"
#include "switch.h"
#include "dns.h"

#define TENMILLISEC 10000	//10 millisecond sleep
#define MAX_NAME_LENGTH 50
#define MAX_ENTRY 256
#define TRUE	1
#define FALSE	0

void dns_main(int host_id) {

struct net_port *node_port_list;
struct net_port **node_port;
int node_port_num;
struct packet *in_packet;
struct packet *new_packet;
struct net_port *p;
struct host_job *new_job, *new_job2;
struct job_queue job_q;

int entries = 0;

//the index of the id maps the id to the domain name.
int id_table[MAX_ENTRY];
char name_table[MAX_ENTRY][MAX_NAME_LENGTH+1]; 

int i, j, n, k;
for(i=0; i<MAX_ENTRY; i++)
	id_table[i] = -1;

//Initialize pipes

node_port_list  = net_get_port_list(host_id);
node_port_num = 0;
for(p=node_port_list; p!=NULL; p=p->next) {
	node_port_num++;
}

node_port = (struct net_port **)
	malloc(node_port_num*sizeof(struct net_port *));

p = node_port_list;
for(k=0; k<node_port_num; k++) {
	node_port[k] = p;
	p = p->next;
}

job_q_init(&job_q);

while(1) {
	for(k=0; k<node_port_num; k++) {
		int_packet = (struct packet *) malloc(sizeof(struct packet));
		n = packet_recv(node_port[k], in packet);

		if((n>0) && ((int) in_packet->dst == host_id)) {
			new_job = (struct host_job *)
				malloc(sizeof(struct host_job));
			new_job->in_port_index = k;
			new_job->packet = in_packet;

			switch(in_packet->type) {
				
				case (char) PKT_DNS_REGISTER:
					new_job->type = JOB_DNS_REGISTER;
					job_q_add(&job_q, new_job);
					break;

				case (char) PKT_DNS_REQ:
					new_job->type = JOB_DNS_REQ;
					job_q_add(&job_q, new_job);
				default:
					free(in_packet);
					free(new_job);
			}
		}
		else
			free(in_packet);		
	}

	if(job_q_num(&job_q) > 0) {
	
		new_job = job_q_remove(&job_q);		
		int id;		
		char name[MAX_ENTRY];

		switch(new_job->type) {
			
			case SEND_PKT_ALL_PORTS:
				for(k=0; k<node_port_num; k++)
					packet_send(node_port[k], new_job->packet);
				
				free(new_job->packet);
				free(new_job);
				break;
		
			//adds host entry to the table. does NOT account for multiple entries from same host
			case JOB_DNS_REGISTER:	
				id = new_job->packet->src;	
				n = sprintf(name, "%s", new_job->packet->payload);
	
				//will add in entry if the table is not full and the name is correct size  
				if(entries < MAX_ENTRY && n <= MAX_NAME_LENGTH) {
					if(n == 256) name[n-1] = '\0';	//will cut off char to fit in EOF
					else	name[n] = '\0';		//if the name is the max length		 
					i = 0;
					while(id_table[i] != -1)
						i++;
					entries++;
					id_table[i] = id; 

					for(j=0; (new_job->packet->payload[j]!='\0') && j<MAX_ENTRY; j++)
						name_table[i][j] == name[j];
					
					new_packet = (struct packet *)malloc(sizeof(struct packet));
					new_packet->dst = new_job->packet->src;
					new_packet->src = (char) host_id;
					new_packet->type = PKT_DNS_REPLY_REG;
					new_packet->length = 0;

					new_job2 = (struct host_job *)malloc(sizeof(struct host_job));
					new_job2->type = JOB_SEND_PKT_ALL_PORTS;
					new_job2->new_packet;

					job_q_add(&job_q, new_job2);
				}	

				free(new_job->packet);	
				free(new_job);	
				break;
			
			//sends physical ID of a given domain name to the requesting host
			//will reply only if the file exists.
			case JOB_DNS_REQ:
				n = sprintf(name, "%s", new_job->packet->payload);
				name[n] = '\0';
						
				int found = FALSE;	
				i = 0;
				while(i<MAX_ENTRY && found == FALSE) {
					if(strcmp(name,name_table[i]) == 0) {
						i--;
						found = TRUE;
					}
					
					i++;
				}
			
				new_packet = (struct packet *)malloc(sizeof(struct packet));
				new_packet->dst = new_job->packet->src;
				new_packet->src = (char) host_id;
				new_packet->type = PKT_DNS_REPLY_REQ;

				//if id_table[i] == -1 then it means that the reocreded name is not associated
				//with an id.
				if(found == TRUE && id_table[i] != -1) {
					new_packet->payload[0] = 'Y';
					new_packet->payload[1] = (char) id_table[i];
					new_packet->payload[] = '\0'; 
					new_packet->length = n+1;
				}
				else {
					new_packet->payload[0] = 'N';
					new_packet->payload[1] = '\0';
					new_packet->length = 2;
				}

				new_job2 = (struct host_job *)malloc(sizeof(struct host_job));
				new_job2->type = JOB_SEND_PKT_ALL_PORTS;
				new_job2->packet = new_packet;
				job_q_add(&job_q, new_job2);

				free(new_job->packet);	
				free(new_job);	
				break;		

			default:
				free(new_job->packet);	
				free(new_job);	
		}
	}

usleep(TENMILLISEC);
}

}

