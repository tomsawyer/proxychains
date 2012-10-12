#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <sys/utsname.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>

#include "core.h"
#include "logger.h"

#define SOCKET_FILE "/tmp/px.sock"

int sock_fd;
int servlen;
struct sockaddr_un  serv_addr;

int init_logger() {

	sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, SOCKET_FILE);

	servlen = strlen(serv_addr.sun_path) +
				 	 sizeof(serv_addr.sun_family);

	return 0;
}

int _send_msg(void *data, int data_size) {
	 sendto(sock_fd, data, data_size, 0, &serv_addr,
			 servlen);
	 return 0;
}

char *random_id(char *buf, int buf_size) {

	const char charset[] = "0123456789abcdef";

	for(int i=0;i<buf_size-1;i++) {
		buf[i] = charset[ (int)( rand()%(sizeof(charset)-1)) ];
	}
	buf[buf_size] = '\0';

	return buf;
}

chain_st *new_chain(uint32_t target_addr, unsigned short target_port) {
	chain_st *ch=calloc(1, sizeof(*ch));
	random_id(ch->id, IDCHAINSIZE+1);
	ch->target_addr = target_addr;
	ch->target_port = target_port;
	ch->status = EVENT_CONNECTING;
	return ch;
}

void free_chain(chain_st *chain) {
	free(chain);
}

int new_chain_event(chain_st *newchain) {
	return 0;
}

int chain_step_event(chain_st *chain, int event, unsigned short proxy_type, uint32_t proxy_addr, unsigned short proxy_port) {

	int i = 0;
	for(;chain->proxies_row[i].addr>0;i++);

	chain->proxies_row[i].type = htons(proxy_type);
	chain->proxies_row[i].addr = htonl(proxy_addr);
	chain->proxies_row[i].port = htons(proxy_port);

	chain->status = event;

	_send_msg(chain, sizeof(*chain));

	return 0;
}

int chain_connected_event(chain_st *chain) {
	chain->status = EVENT_SUCCEED;
	_send_msg(chain, sizeof(*chain));
	return 0;
}

int timeout_event(chain_st *chain) {
	return 0;
}

int disconnect_event(chain_st *chain) {
	return 0;
}

int select_proxy_failed_event(chain_st *chain) {
	chain->status = EVENT_SELECTFAILED;
	_send_msg(chain, sizeof(*chain));
	return 0;
}

int out_of_proies_event(chain_st *chain) {
	chain->status = EVENT_OUTOFPROXIES;
	_send_msg(chain, sizeof(*chain));
	return 0;
}

int chain_cant_connect_event(chain_st *chain) {
	chain->status = EVENT_FAILED;
	_send_msg(chain, sizeof(*chain));
	return 0;
}

int chain_connectin_target_event(chain_st *chain) {
	chain->status = EVENT_TARGET_CONNECTING;
	_send_msg(chain, sizeof(*chain));
	return 0;
}

int send_event(chain_st *chain) {
	return 0;
}
