#ifndef __PACKETS_H__
#define __PACKETS_H__

/* constants */
#define NBR_ROUTER 5
#define NBR_LINK 100
#define INF 100
#define BUF_SIZE 100

/* packets structures*/
struct pkt_HELLO;
struct pkt_LSPDU;
struct pkt_INIT;
struct link_cost;
struct circuit_DB;

int make_RIB(unsigned int rid, int router_fd, struct sockaddr_in *nse_info);

#endif