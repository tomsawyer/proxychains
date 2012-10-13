/*
 * proxychains4monitor.h
 *
 *  Created on: Oct 13, 2012
 *      Author: horn
 */

#ifndef PROXYCHAINS4MONITOR_H_
#define PROXYCHAINS4MONITOR_H_

#include <ncurses.h>
#include "uthash/uthash.h"
#include "logger.h"

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
	case EVENT_NEW_CONNECTION: return "EVENT_NEW_CONNECTION";
	case EVENT_FAILED: return "EVENT_FAILED";
	case EVENT_CONNECTING: return "EVENT_CONNECTING";
	case EVENT_DISCONNECTED: return "EVENT_DISCONNECTED";
	case EVENT_OUTOFPROXIES: return "EVENT_OUT_OF_PROXIES";
	case EVENT_SELECT_FAILED: return "EVENT_SELECT_FAILED";
	case EVENT_SOME_ERROR: return "EVENT_SOME_ERROR";
	case EVENT_SUCCEED: return "EVENT_SUCCEED";
	case EVENT_TIMEOUT: return "EVENT_TIMEOUT";
	case EVENT_AGAIN: return "EVENT_AGAIN";
	case EVENT_TARGET_CONNECTING: return "EVENT_TARGET_CONNECTING";
	case EVENT_DEAD_PROXY: return "EVENT_DEAD_PROXY";
	case EVENT_EXTERN_LOAD: return "EVENT_EXTERN_LOAD";

	default: return "wut?";
	}
}

void ncurses_init_colors() {

	init_pair(EVENT_NEW_CONNECTION+1, COLOR_YELLOW, COLOR_BLACK);
	init_pair(EVENT_CONNECTING+1, COLOR_YELLOW, COLOR_BLACK);
	init_pair(EVENT_TARGET_CONNECTING+1, COLOR_YELLOW, COLOR_BLACK);
	init_pair(EVENT_SUCCEED+1, COLOR_GREEN, COLOR_BLACK);
	init_pair(EVENT_EXTERN_LOAD+1, COLOR_BLACK, COLOR_YELLOW);

	//errors
	init_pair(EVENT_FAILED+1, COLOR_RED, COLOR_BLACK);
	init_pair(EVENT_DISCONNECTED+1, COLOR_WHITE, COLOR_BLACK); //XXX not implemented
	init_pair(EVENT_OUTOFPROXIES+1, COLOR_BLACK, COLOR_RED);
	init_pair(EVENT_SELECT_FAILED+1, COLOR_BLACK, COLOR_RED);
	init_pair(EVENT_DEAD_PROXY+1, COLOR_BLACK, COLOR_RED);
	init_pair(EVENT_SOME_ERROR+1, COLOR_RED, COLOR_BLACK);
	init_pair(EVENT_TIMEOUT+1, COLOR_WHITE, COLOR_RED);
	init_pair(EVENT_AGAIN+1, COLOR_RED, COLOR_BLACK);		//XXX not implemented

}

#endif /* PROXYCHAINS4MONITOR_H_ */
