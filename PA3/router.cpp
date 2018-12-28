#include "utilities.h"
#include "packets.h"
#include <netinet/in.h>
//#include <WinSock2.h>
#include <stdlib.h>
#include <cstdio>
#include <unistd.h>
#include <cstring>

int main(int argc, char **argv) {
	if (argc != 5) {
		printf("Please input 4 arguments: <router_id> <nse_host> <nse_port> <router_port>!\n");
		return -1;
	}
	int rid, nse_ip, nse_port, router_port;
	rid = atoi(argv[1]);
	hostname_to_ip(argv[2], &nse_ip);
	nse_port = atoi(argv[3]);
	router_port = atoi(argv[4]);

	int router_fd = make_and_bind_udp_socket(router_port);

	struct sockaddr_in nse_info;
	make_server_info(nse_ip, nse_port, &nse_info);

	make_RIB(rid, router_fd, &nse_info);
	close(router_fd);
	return 0;
}


