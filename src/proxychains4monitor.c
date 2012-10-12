#undef _GNU_SOURCE
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <ncurses.h>

#include "uthash/uthash.h"
#include "logger.h"

#define SOCK_PATH "/tmp/px.sock"

#define MAXRECSTRLEN sizeof(" -> 123.123.123.123")*(MAXPROXIES+1)+32
#define BUF_SIZE 256

typedef struct record{
	char id[IDCHAINSIZE+1];
	int no;

	chain_st chain;

	char *name_ptr;
	char *descr_ptr;

	time_t time_added;

	UT_hash_handle hh;
} record_t;

char *status_str(int id) {
	switch(id) {
	case EVENT_FAILED: return "EVENT_FAILED";
	case EVENT_CONNECTING: return "EVENT_CONNECTING";
	case EVENT_DISCONNECTED: return "EVENT_DISCONNECTED";
	case EVENT_OUTOFPROXIES: return "EVENT_OUT_OF_PROXIES";
	case EVENT_SELECTFAILED: return "EVENT_SELECT_FAILED";
	case EVENT_SOME_ERROR: return "EVENT_SOME_ERROR";
	case EVENT_SUCCEED: return "EVENT_SUCCEED";
	case EVENT_TIMEOUT: return "EVENT_TIMEOUT";
	case EVENT_AGAIN: return "EVENT_AGAIN";
	case EVENT_TARGET_CONNECTING: return "EVENT_TARGET_CONNECTING";
	default: return "wut?";
	}
}

void ncurses_init_colors() {

	init_pair(EVENT_SUCCEED+1, COLOR_GREEN, COLOR_BLACK);
	init_pair(EVENT_CONNECTING+1, COLOR_YELLOW, COLOR_BLACK);
	init_pair(EVENT_FAILED+1, COLOR_RED, COLOR_BLACK);
	init_pair(EVENT_TARGET_CONNECTING+1, COLOR_YELLOW, COLOR_BLACK);

	init_pair(EVENT_DISCONNECTED+1, COLOR_WHITE, COLOR_BLACK); //XXX not implanted
	init_pair(EVENT_OUTOFPROXIES+1, COLOR_BLACK, COLOR_RED);
	init_pair(EVENT_SELECTFAILED+1, COLOR_BLACK, COLOR_RED);
	init_pair(EVENT_SOME_ERROR+1, COLOR_RED, COLOR_BLACK);
	init_pair(EVENT_TIMEOUT+1, COLOR_WHITE, COLOR_RED);
	init_pair(EVENT_AGAIN+1, COLOR_RED, COLOR_BLACK);
}

char *build_chain_string(const chain_st *rec, char *buf, int buf_size)
{
	int i = 0;
	int nPos = 0;
	uint32_t addr;

	proxy_st proxy = rec->proxies_row[i++];

	nPos = sprintf(buf, "you");

	while(proxy.addr > 0) {
		addr = ntohl(proxy.addr);
		nPos += snprintf(buf+nPos, buf_size-nPos, " -> %s:%d", inet_ntoa(*(struct in_addr*)&addr), ntohs(proxy.port));
		proxy=rec->proxies_row[i++];
	}

	if(rec->status == EVENT_TARGET_CONNECTING || rec->status == EVENT_FAILED || rec->status == EVENT_SUCCEED) {
		addr = ntohl(rec->target_addr);
		nPos += snprintf(buf+nPos, buf_size, " => [%s:%d]", inet_ntoa(*(struct in_addr*)&addr), ntohs(rec->target_port));
	}

	if(rec->status == EVENT_SELECTFAILED)
		nPos+=snprintf(buf+nPos, buf_size, " ->");
	snprintf(buf+nPos, buf_size, " (%s)", status_str(rec->status));

	return 0;
}

record_t *records = NULL;
int total_recs = 0;

int uthash_sort_by_time(record_t *rec1, record_t *rec2) {
	if(rec1->time_added==rec2->time_added)
		return 0;
	return (rec1->time_added>rec2->time_added) ? -1 : 1;
}

record_t *create_record(chain_st chain) {

	char caption[MAXRECSTRLEN];
	build_chain_string((&chain), caption, MAXRECSTRLEN);

	record_t *new = calloc(1,sizeof(*new));

	strcpy(new->id, chain.id);
	new->chain = chain;
	new->name_ptr = strdup(chain.id);
	new->descr_ptr = strdup(caption);

	new->time_added = time(NULL);

	return new;
}

void free_record(record_t *rec) {
	free(rec->descr_ptr);
	free(rec->name_ptr);
	free(rec);
}

void add_new_record(chain_st chain) {

	record_t *old = NULL;
	record_t *new = NULL;

	HASH_FIND_STR(records, chain.id, old);
	new = create_record(chain);

	if(old != NULL) {
		new->no = old->no;
		HASH_DEL(records, old);
		free_record(old);
	} else {
		new->no = total_recs++;
	}

	HASH_ADD_STR(records, id, new);
	HASH_SORT(records,uthash_sort_by_time);
}

#define NCURS 1

int main(int argc, char ** argv)
{
	struct sockaddr_un srvr_name, rcvr_name;
	//char buf[BUF_SIZE];
	int   sock;
	socklen_t   namelen;
	int bytes = 0;
	int rows;

	unlink(SOCK_PATH);

	sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock < 0)  {
		perror("socket failed");
		return EXIT_FAILURE;
	}
	srvr_name.sun_family = AF_UNIX;
	strcpy(srvr_name.sun_path, SOCK_PATH);
	namelen = (socklen_t)(strlen(srvr_name.sun_path) + sizeof(srvr_name.sun_family));

	if (bind(sock, &srvr_name, namelen) < 0)  {
		perror("bind failed");
		return EXIT_FAILURE;
	}

#if NCURS
	initscr();			/* Start curses mode 		  */
	start_color();
	cbreak();
	noecho();

	if(has_colors() == FALSE)
	{	endwin();
		printf("Your terminal does not support color\n");
		exit(1);
	}


	keypad(stdscr, TRUE);
	rows = getmaxy(stdscr);
	ncurses_init_colors();

#endif

	while(1) {
		chain_st chain;


		bytes = recvfrom(sock, &chain, sizeof(chain),  0, &rcvr_name, &namelen);

		if (bytes < 0) {
			perror("recvfrom failed");
			return EXIT_FAILURE;
		}
	    erase();

        add_new_record( chain );

		record_t *rec = (record_t*)NULL;
		int row = 0;
	    for(rec = records; rec != NULL && row<rows; rec=rec->hh.next, row++) {
#if NCURS
	    	attron( COLOR_PAIR( rec->chain.status+1 ) );
	    	printw("(%d) %s\n",rec->no, rec->descr_ptr);
#else
	    	printf("(%d) %s\n",rec->no, rec->descr_ptr);
#endif
	    	//attroff( COLOR_PAIR( rec->chain.status+1 ) );
	    }
	    refresh();
	}

#if NCURS
	endwin();
#endif
	close(sock);
	unlink(SOCK_PATH);
}
 
