/*
 *  Copyright (C) 2012  Samo Pogacnik
 */

/*
 * The hello_evm demo
 */
#ifndef hello_evm_c
#define hello_evm_c
#else
#error Preprocesor macro hello_evm_c conflict!
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <evm/libevm.h>
#include "hello_evm.h"

unsigned int COMP_MODULE = 1;
unsigned int log_mask;
unsigned int use_syslog = 0;

static struct evm_init_struct evs_init;

static struct evm_fds_struct evs_fds = {
	.nfds = 0
};

static struct evm_sigpost_struct evs_sigpost = {
	.sigpost_handle = signal_processing
};

static int signal_processing(int sig, void *ptr)
{
	log_debug("(entry) sig=%d, ptr=%p\n", sig, ptr);
	return 0;
}

static struct evm_link_struct evs_linkage[] = {
	[EV_TYPE_HELLO_MSG].ev_type_parse = NULL,
	[EV_TYPE_HELLO_MSG].ev_type_link = hello_messages_link,
	[EV_TYPE_HELLO_TMR].ev_type_link = helloTmrs_link,
};

static struct evm_tab_struct evm_tbl[] = {
	{ /*HELLO messages*/
		.ev_type = EV_TYPE_HELLO_MSG,
		.ev_id = EV_ID_HELLO_MSG_UNKNOWN,
		.ev_prepare = NULL,
		.ev_handle = evUnknownMsg,
		.ev_finalize = NULL, /*nothing to free*/
	}, {
		.ev_type = EV_TYPE_HELLO_MSG,
		.ev_id = EV_ID_HELLO_MSG_HELLO,
		.ev_prepare = NULL,
		.ev_handle = evHelloMsg,
		.ev_finalize = NULL, /*nothing to free*/
	}, { /*HELLO timers*/
		.ev_type = EV_TYPE_HELLO_TMR,
		.ev_id = EV_ID_HELLO_TMR_IDLE,
		.ev_handle = evHelloTmrIdle,
		.ev_finalize = evm_finalize_timer, /*internal freeing*/
	}, { /*EOT - (End Of Table)*/
		.ev_handle = NULL,
	},
};

/*
 * HELLO messages
 */
/* unknown message */
static struct message_struct unknownMsg = {
	.msg_ids.ev_id = EV_ID_HELLO_MSG_UNKNOWN
};

/* hello message */
static struct message_struct helloMsg = {
	.msg_ids.ev_id = EV_ID_HELLO_MSG_HELLO
};

static int hello_messages_link(int ev_id, int evm_idx)
{
	log_debug("(cb entry) ev_id=%d, evm_idx=%d\n", ev_id, evm_idx);
	switch (ev_id) {
	case EV_ID_HELLO_MSG_UNKNOWN:
		unknownMsg.msg_ids.evm_idx = evm_idx;
		break;
	case EV_ID_HELLO_MSG_HELLO:
		helloMsg.msg_ids.evm_idx = evm_idx;
		break;
	default:
		return -1;
	}
	return 0;
}

/*
 * HELLO timers
 */
static struct timer_struct *helloIdleTmr;
static struct evm_ids helloIdleTmr_evm_ids = {
	.ev_id = EV_ID_HELLO_TMR_IDLE
};

static int helloTmrs_link(int ev_id, int evm_idx)
{
	log_debug("(cb entry) ev_id=%d, evm_idx=%d\n", ev_id, evm_idx);
	switch (ev_id) {
	case EV_ID_HELLO_TMR_IDLE:
		helloIdleTmr_evm_ids.evm_idx = evm_idx;
		break;
	default:
		return -1;
	}
	return 0;
}

static struct timer_struct * hello_startIdle_timer(struct timer_struct *tmr, time_t tv_sec, long tv_nsec, void *ctx_ptr)
{
	log_debug("(entry) tmr=%p, sec=%ld, nsec=%ld, ctx_ptr=%p\n", tmr, tv_sec, tv_nsec, ctx_ptr);
	stop_timer(tmr);
	return start_timer(evm_tbl, helloIdleTmr_evm_ids, tv_sec, tv_nsec, ctx_ptr);
}

/*
 * HELLO event handlers
 */
static int evUnknownMsg(void *ev_ptr)
{
	log_debug("(cb entry) ev_ptr=%p\n", ev_ptr);
	log_verbose("Unknown msg received!\n");

	return 0;
}

static int evHelloMsg(void *ev_ptr)
{
#if 1
	log_debug("(cb entry) ev_ptr=%p\n", ev_ptr);
	log_verbose("HELLO msg received!\n");

	helloIdleTmr = hello_startIdle_timer(helloIdleTmr, 10, 0, NULL);
#else
	/* liveloop */
	evm_message_pass(&helloMsg);
#endif

	return 0;
}

static int evHelloTmrIdle(void *ev_ptr)
{
	int status = 0;

	log_debug("(cb entry) ev_ptr=%p\n", ev_ptr);
	log_verbose("IDLE timer expired!\n");

	evm_message_pass(&helloMsg);

	return status;
}

/*
 * Main HELLO initialization
 */
int main(int argc, char *argv[])
{
	int status = 0;

	/* Prepare dummy FD (used STDIN in this case) for EVM to operate over internal message queue only. */
	evs_fds.ev_type_fds[evs_fds.nfds] = EV_TYPE_HELLO_MSG;
	evs_fds.ev_poll_fds[evs_fds.nfds].fd = 0; /*STDIN*/
	evs_fds.ev_poll_fds[evs_fds.nfds].events = 0 /*POLLIN*/;
	evs_fds.msg_receive[evs_fds.nfds] = NULL;
//	evs_fds.msg_send[evs_fds.nfds] = NULL;
	evs_fds.msg_ptrs[evs_fds.nfds] = (struct message_struct *)calloc(1, sizeof(struct message_struct));
	if (evs_fds.msg_ptrs[evs_fds.nfds] == NULL) {
		errno = ENOMEM;
		return_err("calloc(): 1 times %zd bytes\n", sizeof(struct message_struct));
	}
	evs_fds.msg_ptrs[evs_fds.nfds]->fds_index = evs_fds.nfds;
	evs_fds.nfds++;

	/* Initialize event machine... */
	evs_init.evm_sigpost = &evs_sigpost;;
	evs_init.evm_link = evs_linkage;
	evs_init.evm_link_max = sizeof(evs_linkage) / sizeof(struct evm_link_struct) - 1;
	evs_init.evm_tab = evm_tbl;
	evs_init.evm_fds = &evs_fds;
	log_debug("evs_linkage index size = %d\n", evs_init.evm_link_max);
	if ((status = evm_init(&evs_init)) < 0) {
		exit(status);
	}

	/* Set initial timer */
	helloIdleTmr = hello_startIdle_timer(NULL, 1, 0, NULL);

	/*
	 * Main EVM processing (event loop)
	 */
	status = evm_run(&evs_init);

	exit(status);
}

