/*
 * switch.h
 */

struct forwarding_table_entry {
	int valid;
	int host_id;
	int port;
};

struct forwarding_table {

};

struct network_switch {
	int port_number;
	int port_count;
};
