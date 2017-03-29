/*
 * switch.h
 */

struct forwarding_table_entry {
	int valid;
	int dst_host_id;
	int port;
};

void switch_main(int host_id);
