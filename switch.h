/*
 * switch.h
 */

typedef struct{ 
	int valid;
	int dst_switch_id;
	int port;
} switch_forwarding_table;

int containsEntry(switch_forwarding_table forwarding_table[], char dst, int number_of_entries);
void switch_main(int host_id);
