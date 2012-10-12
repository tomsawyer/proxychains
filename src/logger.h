#ifndef LOGGER_H_
#define LOGGER_H_

#define MAXPROXIES 16
#define IDCHAINSIZE 16

typedef struct proxy_struct{
	int type;
	uint32_t addr;
	unsigned short port;
} proxy_st;

enum {
	EVENT_SUCCEED,
	EVENT_CONNECTING,
	EVENT_TIMEOUT,
	EVENT_DISCONNECTED,
	EVENT_SELECTFAILED,
	EVENT_FAILED,
	EVENT_SOME_ERROR,
	EVENT_OUTOFPROXIES,
	EVENT_AGAIN,
	EVENT_TARGET_CONNECTING
};

typedef struct chain_struct{
	char id[IDCHAINSIZE+1];

	uint32_t target_addr;
	unsigned short int target_port;
	proxy_st proxies_row[MAXPROXIES];
	unsigned short status;
} chain_st;

int init_logger();

chain_st *new_chain(uint32_t target_addr, unsigned short target_port);
char *random_id(char *buf, int buf_size);
void free_chain(chain_st *chain);
int new_chain_event(chain_st *newchain);
int chain_step_event(chain_st *chain, int event, unsigned short proxy_type,
									uint32_t proxy_addr, unsigned short proxy_port);
int chain_connected_event(chain_st *chain);
int timeout_event(chain_st *chain);
int disconnect_event(chain_st *chain);
int select_proxy_failed_event(chain_st *chain);
int chain_cant_connect_event(chain_st *chain);
int send_event(chain_st *chain);
int chain_connectin_target_event(chain_st *chain);
int out_of_proies_event(chain_st *chain);

#endif /* LOGGER_H_ */
