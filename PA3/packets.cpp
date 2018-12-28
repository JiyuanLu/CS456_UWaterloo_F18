#include "packets.h"
#include "utilities.h"
#include <netinet/in.h>
//#include <WinSock2.h>
#include <stdlib.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>


struct pkt_HELLO
{
	unsigned int router_id;		/* id of the router who sends the HELLO PDU */
	unsigned int link_id;		/* id of the link through which it is sent */
};

struct pkt_LSPDU
{
	unsigned int sender;		/* sender of the LS PDU */
	unsigned int router_id;		/* (originl) router id */
	unsigned int link_id;		/* (original) link id */
	unsigned int cost;			/* cost of the link */
	unsigned int via;			/* id of the (current) link through which the LS PDU is sent */
};

struct pkt_INIT
{
	unsigned int router_id;		/* id of the router who sends the INIT PDU */
};

struct link_cost
{
	unsigned int link;			/* link id */
	unsigned int cost;			/* associated cost */
};

struct circuit_DB
{
	unsigned int nbr_link;		/* number of links attached to a router */
	struct link_cost linkcost[NBR_ROUTER];
	/* we assume that at most NBR_ROUTER links are attached to each router */
};

/* Router sends an INIT packet to the NSE */
static int send_INIT(unsigned int rid, int router_fd, struct sockaddr_in *nse_info, FILE *log) {
	struct pkt_INIT pkt = { rid };
	fprintf(log, "R%d sends an INIT: router_id %d\n", rid, pkt.router_id);
	return sendto(router_fd, (const char*) &pkt, sizeof(pkt), 0, (struct sockaddr*) nse_info, sizeof(struct sockaddr_in));
}

/* Router receives Circuit-DB from nse, and updates its link state DB(LSDB) */

/* Router sends a HELLO packet to each of its known neighbors, according to Circuit-DB received in the previous step */
static int send_HELLO(unsigned int rid, int router_fd, unsigned int link_id, struct sockaddr_in *nse_info, FILE *log) {
	struct pkt_HELLO pkt = { rid, link_id };
	fprintf(log, "R%d sends a HELLO: router id %d link id %d\n", rid, pkt.router_id, link_id);
	return sendto(router_fd, (const char*) &pkt, sizeof(pkt), 0, (struct sockaddr*) nse_info, sizeof(struct sockaddr_in));
}

/* Router replies with LS_PDU packets upon receiving HELLO packet from its neighbour */
static void reply_LSPDU(unsigned int rid, int router_fd, unsigned int link_id, struct sockaddr_in *nse_info, struct circuit_DB topology[NBR_ROUTER], FILE *log) {
	struct pkt_LSPDU pkt;
	pkt.sender = rid;
	pkt.router_id = rid;
	pkt.via = link_id;
	for (unsigned int i = 0; i < topology[rid].nbr_link; ++i) {
		pkt.link_id = topology[rid].linkcost[i].link;
		pkt.cost = topology[rid].linkcost[i].cost;
		fprintf(log, "R%d sends a LSPDU: router id %d link id %d cost %d via %d\n", pkt.sender, pkt.router_id, pkt.link_id, pkt.cost, pkt.via);
		sendto(router_fd, (const char*)&pkt, sizeof(pkt), 0, (struct sockaddr*) nse_info, sizeof(struct sockaddr_in));
	}
}

/* Router also forwards LS_PDU packet to each of its rest neighbors upon receiving LS_PDU packet*/
static void forward_LSPDU(unsigned int rid, int router_fd, int src, unsigned int original_link, int original_link_cost, struct sockaddr_in *nse_info, struct circuit_DB topology[NBR_ROUTER], bool helloed[NBR_ROUTER + 1], FILE *log) {
	struct pkt_LSPDU pkt;
	pkt.sender = rid;
	// forward LS_PDU on each link the router connects to(except the one it received the LS_PDU from)
	for (unsigned int i = 0; i < topology[rid].nbr_link; ++i) {
		pkt.router_id = src;
		pkt.link_id = original_link;
		pkt.cost = original_link_cost;
		pkt.via = topology[rid].linkcost[i].link;
		if (pkt.via == original_link || !helloed[i])
			continue;
		fprintf(log, "R%d sends a LSPDU: router id %d link id %d cost %d via %d\n", pkt.sender, pkt.router_id, pkt.link_id, pkt.cost, pkt.via);
		sendto(router_fd, (const char*)&pkt, sizeof(pkt), 0, (struct sockaddr*) nse_info, sizeof(struct sockaddr_in));
	}
}

/* Make an adjacency matrix fromm the topology db*/
static void make_adjacency_matrix(struct circuit_DB topology[NBR_ROUTER], bool adj[NBR_ROUTER + 1][NBR_ROUTER + 1], unsigned int cost[NBR_ROUTER + 1][NBR_ROUTER + 1]) {
	int links[NBR_LINK] = { 0 };
	for (unsigned int i = 0; i <= NBR_ROUTER; ++i) {
		for (unsigned int j = 0; j <= NBR_ROUTER; ++j) {
			adj[i][j] = 0;
			cost[i][j] = 0;
		}
	}

	for (unsigned int i = 1; i <= NBR_ROUTER; ++i) {
		for (unsigned int j = 0; j < topology[i].nbr_link; ++j) {
			int link = topology[i].linkcost[j].link;
			int dest = links[link];
			if (dest != 0) {
				adj[i][dest] = true;
				adj[dest][i] = true;
				cost[i][dest] = topology[i].linkcost[j].cost;
				cost[dest][i] = topology[i].linkcost[j].cost;
			}
			else {
				links[link] = i;
			}
		}
	}
}

static void dijkstra(struct circuit_DB topology[NBR_ROUTER], unsigned int rid, bool known[NBR_ROUTER + 1], FILE *log) {
	bool adj[NBR_ROUTER + 1][NBR_ROUTER + 1];
	unsigned int cost[NBR_ROUTER + 1][NBR_ROUTER + 1];
	make_adjacency_matrix(topology, adj, cost);
	unsigned int d[NBR_ROUTER + 1] = { 0 };
	unsigned int p[NBR_ROUTER + 1] = { 0 };

	// initialization
	for (unsigned int i = 1; i <= NBR_ROUTER; ++i) {
		if (adj[rid][i]) {
			d[i] = cost[rid][i];
			p[i] = rid;
		}
		else {
			d[i] = INF;
		}
	}
	d[rid] = 0;

	int unvisited = NBR_ROUTER;
	bool visited[NBR_ROUTER + 1] = { false };
	while (unvisited > 0) {
		unsigned int distance = INF;
		unsigned int next = 0;

		for (unsigned int i = 1; i < NBR_ROUTER; ++i) {
			if (!visited[i] && d[i] <= distance) {
				next = i;
				distance = d[i];
			}
		}
		unvisited--;

		if (next == 0)
			break;

		visited[next] = true;

		for (unsigned int i = 1; i <= NBR_ROUTER; ++i) {
			if (adj[next][i] && !visited[i]) {
				unsigned int dt = d[next] + cost[next][i];
				if (dt < d[i]) {
					d[i] = dt;
					p[i] = next;
				}
			}
		}
	}

	fprintf(log, "# RIB\n");
	for (unsigned int i = 1; i <= NBR_ROUTER; ++i) {
		if (!known[i]) {
			continue;
		}
		if (i == rid) {
			fprintf(log, "R%d -> R%d -> (Local), 0\n", rid, i);
		}
		else {
			if (d[i] == INF) {
				fprintf(log, "R%d -> R%d -> INFINITY\n", rid, i);
				continue;
			}
			unsigned int pv = i;
			unsigned int ppv = i;
			while (true) {
				pv = p[pv];
				if (pv == rid || pv == 0)
					break;
				ppv = pv;
			}
			fprintf(log, "R%d -> R%d -> R%d, %d\n", rid, i, ppv, d[i]);
		}
	}
	fprintf(log, "\n");
}

static bool update_topology(struct circuit_DB topology[NBR_ROUTER], struct pkt_LSPDU *pkt) {
	struct circuit_DB *db = &topology[pkt->router_id];
	// if the record already exists in the current router's topology , do nothing, return false
	for (unsigned int i = 0; i < db->nbr_link; ++i) {
		if (db->linkcost[i].link == pkt->link_id)
			return false;
	}

	db->linkcost[db->nbr_link].link = pkt->link_id;
	db->linkcost[db->nbr_link].cost = pkt->cost;
	db->nbr_link++;
	return true;
}



static void log_topology(unsigned int rid, struct circuit_DB topology[NBR_ROUTER + 1], FILE *log) {
	fprintf(log, "\n# Topology Database\n");
	for (unsigned int i = 1; i <= NBR_ROUTER; ++i) {
		if (topology[i].nbr_link <= 0)
			continue;
		fprintf(log, "R%d -> R%d nbr link %d\n", rid, i, topology[i].nbr_link);
		for (unsigned int j = 0; j < topology[i].nbr_link; ++j) {
			fprintf(log, "R%d -> R%d link %d cost %d\n", rid, i, topology[i].linkcost[j].link, topology[i].linkcost[j].cost);
		}
	}
	fprintf(log, "\n");
}

int make_RIB(unsigned int rid, int router_fd, struct sockaddr_in *nse_info) {
	struct circuit_DB db;
	bool helloed[NBR_ROUTER + 1] = { false };
	bool known[NBR_ROUTER + 1] = { false };
	known[rid] = true;
	FILE *log;
	char buf[1024] = { 0 };
	snprintf(buf, sizeof(buf), "router%d.log", rid);
	log = fopen(buf, "w");
	setbuf(log, NULL);

	send_INIT(rid, router_fd, nse_info, log);
	
	// receive circuit_DB packet
	recvfrom(router_fd, (char*) &db, sizeof(db), 0, NULL, NULL);
	fprintf(log, "R%d receives a circuit_DB: nbr_link %d\n", rid, db.nbr_link);

	for (unsigned int i = 0; i < db.nbr_link; ++i) {
		send_HELLO(rid, router_fd, db.linkcost[i].link, nse_info, log);
	}

	memset(buf, 0, sizeof(buf));
	struct circuit_DB topology[NBR_ROUTER + 1];
	memset(topology, 0, sizeof(topology));
	topology[rid] = db;
	
	// Log the initial Circuit DB
	log_topology(rid, topology, log);

	// Wait for HELLO packets and then LSPDU packets
	while (true) {
		int length = recvfrom(router_fd, (char*)buf, sizeof(buf), 0, NULL, NULL);
		if (length == sizeof(struct pkt_HELLO)) {
			struct pkt_HELLO *pkt = (struct pkt_HELLO *) buf;
			unsigned int router_id = pkt->router_id;
			unsigned int link_id = pkt->link_id;
			helloed[router_id] = true;
			known[router_id] = true;
			fprintf(log, "R%d receives an HELLO: router_id %d link_id %d\n", rid, router_id, link_id);
			reply_LSPDU(rid, router_fd, link_id, nse_info,  topology, log);
		}
		else if (length == sizeof(struct pkt_LSPDU)) {
			struct pkt_LSPDU *pkt = (struct pkt_LSPDU *) buf;
			unsigned int sender = pkt->sender;
			unsigned int router_id = pkt->router_id;
			unsigned int link_id = pkt->link_id;
			unsigned int cost = pkt->cost;
			unsigned int via = pkt->via;
			known[router_id] = true;
			fprintf(log, "R%d receives an LSPDU: sender R%d, router_id %d, link_id %d, cost %d, via %d\n", rid, sender, router_id, link_id, cost, via);
			
			// update topology DB
			// continue if not modified
			if (!update_topology(topology, pkt)) {
				continue;
			}

			// log if modified
			log_topology(rid, topology, log);

			// run Dijkstra
			dijkstra(topology, rid, known, log);

			// forward the LSPDU packet to all neighbors except the origin
			forward_LSPDU(rid, router_fd, router_id, link_id, cost, nse_info, topology, helloed, log);
		}
	}

	
}