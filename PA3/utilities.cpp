#include "utilities.h"
#include <netinet/in.h>
//#include <WinSock2.h>
#include <cstring>
#include <stdlib.h>
#include <cstdio>
#include <netdb.h>
#include <unistd.h>


void hostname_to_ip(char *hostname, int *ip) {
	struct hostent* host_entity = gethostbyname(hostname);
	memcpy(ip, host_entity->h_addr_list[0], host_entity->h_length);
}

int make_socket(int type) {
	int fd = socket(AF_INET, type, 0);
	return fd;
}

int make_and_bind_udp_socket(int port) {
	int fd = make_socket(SOCK_DGRAM);
	struct sockaddr_in host_info = { 0 };
	host_info.sin_family = AF_INET;
	host_info.sin_port = htons(port);
	host_info.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(fd, (struct sockaddr*) &host_info, sizeof(host_info));
	return fd;
}

void make_server_info(int ip, int port, struct sockaddr_in *server_info) {
	memset(server_info, 0, sizeof(struct sockaddr_in));
	server_info->sin_family = AF_INET;
	server_info->sin_port = htons(port);
	memcpy(&server_info->sin_addr, &ip, sizeof(int));
}

