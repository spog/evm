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
#include <getopt.h>

#include "hello_evm.h"

/*
 * The MAIN part.
 */
unsigned int log_mask;
unsigned int evmlog_normal = 1;
unsigned int evmlog_verbose = 0;
unsigned int evmlog_trace = 0;
unsigned int evmlog_debug = 0;
unsigned int evmlog_use_syslog = 0;
unsigned int evmlog_add_header = 1;

static void usage_help(char *argv[])
{
	printf("Usage:\n");
	printf("\t%s [options]\n", argv[0]);
	printf("options:\n");
	printf("\t-q, --quiet              Disable all output.\n");
	printf("\t-t, --trace              Enable trace output.\n");
	printf("\t-v, --verbose            Enable verbose output.\n");
	printf("\t-g, --debug              Enable debug output.\n");
	printf("\t-n, --no-header          No EVMLOG header added to evm_log_... output.\n");
	printf("\t-s, --syslog             Enable syslog output (instead of stdout, stderr).\n");
	printf("\t-h, --help               Displays this text.\n");
}

static int usage_check(int argc, char *argv[])
{
	int c;

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"quiet", 0, 0, 'q'},
			{"trace", 0, 0, 't'},
			{"verbose", 0, 0, 'v'},
			{"debug", 0, 0, 'g'},
			{"no-header", 0, 0, 'n'},
			{"syslog", 0, 0, 's'},
			{"help", 0, 0, 'h'},
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "qtvgnsh", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'q':
			evmlog_normal = 0;
			break;

		case 't':
			evmlog_trace = 1;
			break;

		case 'v':
			evmlog_verbose = 1;
			break;

		case 'g':
			evmlog_debug = 1;
			break;

		case 'n':
			evmlog_add_header = 0;
			break;

		case 's':
			evmlog_use_syslog = 1;
			break;

		case 'h':
			usage_help(argv);
			exit(EXIT_SUCCESS);

		case '?':
			exit(EXIT_FAILURE);
			break;

		default:
			printf("?? getopt returned character code 0%o ??\n", c);
			exit(EXIT_FAILURE);
		}
	}

	if (optind < argc) {
		printf("non-option ARGV-elements: ");
		while (optind < argc)
			printf("%s ", argv[optind++]);
		printf("\n");
		exit(EXIT_FAILURE);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	usage_check(argc, argv);

	log_mask = LOG_MASK(LOG_EMERG) | LOG_MASK(LOG_ALERT) | LOG_MASK(LOG_CRIT) | LOG_MASK(LOG_ERR);

	/* Setup LOG_MASK according to startup arguments! */
	if (evmlog_normal) {
		log_mask |= LOG_MASK(LOG_WARNING);
		log_mask |= LOG_MASK(LOG_NOTICE);
	}
	if ((evmlog_verbose) || (evmlog_trace))
		log_mask |= LOG_MASK(LOG_INFO);
	if (evmlog_debug)
		log_mask |= LOG_MASK(LOG_DEBUG);

	setlogmask(log_mask);

	if (hello_evm_init() < 0)
		exit(EXIT_FAILURE);

	if (hello_evm_run() < 0)
		exit(EXIT_FAILURE);

	exit(EXIT_SUCCESS);
}

/*
 * The EVM part.
 */
#include <evm/libevm.h>

static struct evm_init_struct evs_init;

static struct evm_fds_struct evs_fds = {
	.nfds = 0
};

static struct evm_sigpost_struct evs_sigpost = {
	.sigpost_handle = signal_processing
};

static int signal_processing(int sig, void *ptr)
{
	evm_log_info("(entry) sig=%d, ptr=%p\n", sig, ptr);
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

/* HELLO messages */
static char *hello_str = "HELLO";
static struct message_struct helloMsg = {
	.msg_ids.ev_id = EV_ID_HELLO_MSG_HELLO,
};

static int hello_messages_link(int ev_id, int evm_idx)
{
	evm_log_info("(cb entry) ev_id=%d, evm_idx=%d\n", ev_id, evm_idx);
	switch (ev_id) {
	case EV_ID_HELLO_MSG_HELLO:
		helloMsg.msg_ids.evm_idx = evm_idx;
		break;
	default:
		return -1;
	}
	return 0;
}

/* HELLO timers */
static struct timer_struct *helloIdleTmr;
static struct evm_ids helloIdleTmr_evm_ids = {
	.ev_id = EV_ID_HELLO_TMR_IDLE
};

static int helloTmrs_link(int ev_id, int evm_idx)
{
	evm_log_info("(cb entry) ev_id=%d, evm_idx=%d\n", ev_id, evm_idx);
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
	evm_log_info("(entry) tmr=%p, sec=%ld, nsec=%ld, ctx_ptr=%p\n", tmr, tv_sec, tv_nsec, ctx_ptr);
	stop_timer(tmr);
	return start_timer(evm_tbl, helloIdleTmr_evm_ids, tv_sec, tv_nsec, ctx_ptr);
}

/* HELLO event handlers */
static int evHelloMsg(void *ev_ptr)
{
	struct message_struct *msg = (struct message_struct *)ev_ptr;
#if 1
	evm_log_info("(cb entry) ev_ptr=%p\n", ev_ptr);
	evm_log_notice("HELLO msg received: \"%s\"\n", msg->recv_buff);

	helloIdleTmr = hello_startIdle_timer(helloIdleTmr, 10, 0, NULL);
#else
	/* liveloop */
	evm_message_pass(&helloMsg);
#endif

	return 0;
}

static int evHelloTmrIdle(void *ev_ptr)
{
	static unsigned int count;
	int status = 0;

	evm_log_info("(cb entry) ev_ptr=%p\n", ev_ptr);
	evm_log_notice("IDLE timer expired!\n");

	count++;
	sprintf(helloMsg.recv_buff, "%s: %u", hello_str, count);
	evm_message_pass(&helloMsg);

	return status;
}

/* EVM initialization */
static int hello_evm_init(void)
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
		evm_log_return_err("calloc(): 1 times %zd bytes\n", sizeof(struct message_struct));
	}
	evs_fds.msg_ptrs[evs_fds.nfds]->fds_index = evs_fds.nfds;
	evs_fds.nfds++;

	/* Initialize event machine... */
	evs_init.evm_sigpost = &evs_sigpost;;
	evs_init.evm_link = evs_linkage;
	evs_init.evm_link_max = sizeof(evs_linkage) / sizeof(struct evm_link_struct) - 1;
	evs_init.evm_tab = evm_tbl;
	evs_init.evm_fds = &evs_fds;
	evm_log_debug("evs_linkage index size = %d\n", evs_init.evm_link_max);
	if ((status = evm_init(&evs_init)) < 0) {
		return status;
	}

	return 0;
}

/* Main core processing (event loop) */
static int hello_evm_run(void)
{
	/* Set initial timer */
	helloIdleTmr = hello_startIdle_timer(NULL, 1, 0, NULL);

	/*
	 * Main EVM processing (event loop)
	 */
	return evm_run(&evs_init);
}

