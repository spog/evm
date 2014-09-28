/*
 *  Copyright (C) 2014  Samo Pogacnik
 */

/*
 * The hello_evm demo:
 * This demo shows minimal event machine usage within a single process. It sends
 * HELLO message to itself on every timeout expiration.
 * 1. The MAIN part shows standard C program initialization with options for
 *    different logging capabilities of EVM.
 * 2. The EVM part demonstrates EVM initialization. 
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
	printf("\t-v, --verbose            Enable verbose output.\n");
#if (EVMLOG_MODULE_TRACE != 0)
	printf("\t-t, --trace              Enable trace output.\n");
#endif
#if (EVMLOG_MODULE_DEBUG != 0)
	printf("\t-g, --debug              Enable debug output.\n");
#endif
	printf("\t-s, --syslog             Enable syslog output (instead of stdout, stderr).\n");
	printf("\t-n, --no-header          No EVMLOG header added to every evm_log_... output.\n");
	printf("\t-h, --help               Displays this text.\n");
}

static int usage_check(int argc, char *argv[])
{
	int c;

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"quiet", 0, 0, 'q'},
			{"verbose", 0, 0, 'v'},
#if (EVMLOG_MODULE_TRACE != 0)
			{"trace", 0, 0, 't'},
#endif
#if (EVMLOG_MODULE_DEBUG != 0)
			{"debug", 0, 0, 'g'},
#endif
			{"no-header", 0, 0, 'n'},
			{"syslog", 0, 0, 's'},
			{"help", 0, 0, 'h'},
			{0, 0, 0, 0}
		};

#if (EVMLOG_MODULE_TRACE != 0) && (EVMLOG_MODULE_DEBUG != 0)
		c = getopt_long(argc, argv, "qvtgnsh", long_options, &option_index);
#elif (EVMLOG_MODULE_TRACE == 0) && (EVMLOG_MODULE_DEBUG != 0)
		c = getopt_long(argc, argv, "qvgnsh", long_options, &option_index);
#elif (EVMLOG_MODULE_TRACE != 0) && (EVMLOG_MODULE_DEBUG == 0)
		c = getopt_long(argc, argv, "qvtnsh", long_options, &option_index);
#else
		c = getopt_long(argc, argv, "qvnsh", long_options, &option_index);
#endif
		if (c == -1)
			break;

		switch (c) {
		case 'q':
			evmlog_normal = 0;
			break;

		case 'v':
			evmlog_verbose = 1;
			break;

#if (EVMLOG_MODULE_TRACE != 0)
		case 't':
			evmlog_trace = 1;
			break;
#endif

#if (EVMLOG_MODULE_DEBUG != 0)
		case 'g':
			evmlog_debug = 1;
			break;
#endif

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

/*
 * General EVM structure - required by evm_init() and evm_run():
 */
static struct evm_init_struct evs_init;

/*
 * File descriptors structure - required by evm_fd_add():
 */
static struct evm_fd_struct evs_fd;

/*
 * Signal post-processing callback - optional for evm_init():
 * Covers SIGHUP and SIGCHLD
 */
static struct evm_sigpost_struct evs_sigpost = {
	.sigpost_handle = signal_processing
};

static int signal_processing(int sig, void *ptr)
{
	evm_log_info("(entry) sig=%d, ptr=%p\n", sig, ptr);
	return 0;
}

/*
 * Event types table - required by evm_init():
 * Per event type parser and linkage callbacks
 */
static struct evm_link_struct evs_linkage[] = {
	[EV_TYPE_HELLO_MSG].ev_type_parse = NULL,
	[EV_TYPE_HELLO_MSG].ev_type_link = hello_messages_link,
	[EV_TYPE_HELLO_TMR].ev_type_link = helloTmrs_link,
};

/*
 * Events table - required by evm_init():
 * Messages and timers - their individual IDs and callbacks
 */
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
static char msg_buff[MAX_BUFF_SIZE];
static char *hello_str = "HELLO";
static struct message_struct helloMsg = {
	.msg_ids.ev_id = EV_ID_HELLO_MSG_HELLO,
	.iov_buff.iov_base = (void *)msg_buff,
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
	evm_log_notice("HELLO msg received: \"%s\"\n", (char *)msg->iov_buff.iov_base);

	helloIdleTmr = hello_startIdle_timer(helloIdleTmr, 10, 0, NULL);
#else
	/* liveloop - 100 %CPU usage */
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
	sprintf((char *)helloMsg.iov_buff.iov_base, "%s: %u", hello_str, count);
	evm_message_pass(&helloMsg);

	return status;
}

/* EVM initialization */
static int hello_evm_init(void)
{
	int status = 0;

	evm_log_info("(entry)\n");

	/* Initialize event machine... */
	evs_init.evm_sigpost = &evs_sigpost;;
	evs_init.evm_link = evs_linkage;
	evs_init.evm_link_max = sizeof(evs_linkage) / sizeof(struct evm_link_struct) - 1;
	evs_init.evm_tab = evm_tbl;
	evs_init.evm_epoll_max_events = MAX_EPOLL_EVENTS_PER_RUN;
	evm_log_debug("evs_linkage index size = %d\n", evs_init.evm_link_max);
	if ((status = evm_init(&evs_init)) < 0) {
		evm_log_error("evm_init() failed!\n");
		return status;
	}
	evm_log_debug("evm epoll FD is %d\n", evs_init.evm_epollfd);

	/* Prepare dummy FD (used STDIN in this case) for EVM to operate over internal message queue only. */
	evs_fd.fd = 0; /*STDIN*/
	evs_fd.ev_type = EV_TYPE_HELLO_MSG;
	evs_fd.ev_epoll.events = 0 /*EPOLLPRI*/ /*EPOLLIN*/;
	evs_fd.ev_epoll.data.ptr = (void *)&evs_fd /*our own address*/;
	evs_fd.msg_receive = NULL;
//	evs_fd.msg_send = NULL;
	if ((status = evm_message_fd_add(&evs_init, &evs_fd)) < 0) {
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

