#ifndef __UTILITIES_H__
#define __UTILITIES_H__

void hostname_to_ip(char *hostname, int *ip);
int make_socket(int type);
int make_and_bind_udp_socket(int port);
void make_server_info(int ip, int port, struct sockaddr_in *server_info);

#endif
