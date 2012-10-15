#undef _GNU_SOURCE
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <utime.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <ncurses.h>

#include "uthash/uthash.h"
#include "logger.h"
#include "proxychains4monitor.h"

#define SOCK_PATH "/tmp/px.sock"
#define NCURS 1

record_t *records = NULL;
record_t *events = NULL;

int total_recs = 0;
int total_events = 0;

int events_stats[12];
int chain_length_stats[MAXPROXIES];

int new_conns = 0;
time_t ptime = 0;
int conns_rate = 0;

// build a string that matches chain connection
int build_chain_string(const chain_st *chain, char *buf, int buf_size)
{
	int i = 0;
	int nPos = 0;
	uint32_t addr;

	char arrow[sizeof("->")];

	proxy_st proxy = chain->proxies_vector[i++];

	nPos = sprintf(buf, "you");

	// XXX ntohs(port) ?
	addr = chain->target_addr;
	if(chain->event == EVENT_NEW_CONNECTION) {
		nPos+=snprintf(buf+nPos, buf_size-nPos, " => [%s:%d]", inet_ntoa(*(struct in_addr*)&addr), ntohs(chain->target_port));
		return nPos;
	}

	while(proxy.addr > 0) {

		strcpy(arrow, "->");

		addr = ntohl(proxy.addr);
		nPos += snprintf(buf+nPos, buf_size-nPos, " %s %s:%d", arrow, inet_ntoa(*(struct in_addr*)&addr), ntohs(proxy.port));

		proxy=chain->proxies_vector[i++];
	}

	//print target address info
	if(chain->event == EVENT_TARGET_CONNECTING || chain->event == EVENT_FAILED || chain->event == EVENT_SUCCEED) {
		addr = ntohl(chain->target_addr);
		nPos += snprintf(buf+nPos, buf_size-nPos, " => [%s:%d]", inet_ntoa(*(struct in_addr*)&addr), ntohs(chain->target_port));
	}

	//
	if(chain->event == EVENT_SELECT_FAILED)
		nPos+=snprintf(buf+nPos, buf_size-nPos, " ->");

	snprintf(buf+nPos, buf_size-nPos, " (%s) (%s)", status_str(chain->event), chain->info);

	return nPos;
}

int chain_length(chain_st *chain) {
	int i = 0;
	for(;chain->proxies_vector[i].addr>0;i++);
	return i;
}

int stats_process(chain_st *chain) {

	time_t ctime = time(NULL);

    events_stats[chain->event]++;

    if(chain->event == EVENT_TARGET_CONNECTING)
    	chain_length_stats[chain_length(chain)]++;

    ptime = time;

	return 0;
}

int stats_lengths_str(char *buf, int buf_size) {

	int n = sizeof(chain_length_stats)/sizeof(chain_length_stats[0]);
	int nPos = 0;

	for(int i=0;i<n;i++) {
		if(chain_length_stats[i] > 0)
			nPos+=snprintf(buf+nPos, buf_size-nPos, "%d[%d] ", i, chain_length_stats[i]);
	}

	return nPos;
}

//TODO different sort functions

int uthash_sort_by_time(record_t *rec1, record_t *rec2) {
	if(rec1->time_added==rec2->time_added)
		return 0;
	return (rec1->time_added>rec2->time_added) ? -1 : 1;
}

int uthash_sort_by_no(record_t *rec1, record_t *rec2) {
	if(rec1->no==rec2->no)
		return 0;
	return (rec1->no>rec2->no) ? -1 : 1;
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

record_t *add_new_record(chain_st chain) {

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

	return new;
}

record_t *log_new_record(chain_st chain) {
	record_t *new = create_record(chain);
	new->no = total_events++;
	HASH_ADD_INT(events,no, new);
	HASH_SORT(events,uthash_sort_by_no);
	return new;
}

#define EVENTS_LOG_ROWS 20

int main(int argc, char ** argv)
{
	struct sockaddr_un srvr_name, rcvr_name;

	int   sock;
	socklen_t   namelen;
	int bytes = 0;
	int rows = 50;
	bzero(events_stats, sizeof(events_stats));

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
#if NCURS
	    erase();
#else
	    system ( "clear" );
#endif
        add_new_record( chain );
        log_new_record(chain);
        stats_process(&chain);

		record_t *rec = (record_t*)NULL;
		int row = 0;
	    for(rec = records; rec != NULL && row<rows - EVENTS_LOG_ROWS-1-5; rec=rec->hh.next, row++) {
#if NCURS
	    	attron( COLOR_PAIR( rec->chain.event+1 ) );
	    	printw("(%d) %s\n",rec->no, rec->descr_ptr);
#else
	    	printf("(%d) %s\n",rec->no, rec->descr_ptr);
#endif
	    	//attroff( COLOR_PAIR( rec->chain.status+1 ) );
	    }

	    printw("\n");

	    for(row = 0, rec = events;rec!=NULL && row<EVENTS_LOG_ROWS; rec=rec->hh.next, row++) {
	    	attron( COLOR_PAIR( rec->chain.event+1 ) );
	    	printw("(%d) %s\n",rec->no, rec->descr_ptr);
	    }

	    attron( COLOR_PAIR( EVENT_SUCCEED+1) );
	    char len_stats[256];
	    len_stats[0] = '\0';

	    stats_lengths_str(len_stats, sizeof(len_stats));

	    mvprintw(rows-2,0,"events [success: %d]\t[failed: %d]\t[select_failed:"\
	    		" %d]\t[timeouts: %d]\t[chain retries: %d]\t[dead: %d]",
	    		events_stats[EVENT_SUCCEED], events_stats[EVENT_FAILED],
	    		events_stats[EVENT_SELECT_FAILED],
	    		events_stats[EVENT_TIMEOUT], events_stats[EVENT_AGAIN],
	    		events_stats[EVENT_DEAD_PROXY]);
	    mvprintw(rows-1,0,"chain lengths: %s", len_stats);

	    refresh();
	}

#if NCURS
	endwin();
#endif
	close(sock);
	unlink(SOCK_PATH);
}
 
