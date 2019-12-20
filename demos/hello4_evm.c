/*
 * The hello4_evm demo program
 *
 * This file is part of the "evm" software project which is
 * provided under the Apache license, Version 2.0.
 *
 *  Copyright 2019 Samo Pogacnik <samo_pogacnik@t-2.net>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
*/

/*
 * This demo shows sending message between one and many threads. The initial
 * thread spawns many other threads and sets QUIT timer. Each other thread
 * sends the first HELLO message to the first thread. Every received
 * message in any thread sets a new timeout and another HELLO message is
 * sent back to the sender thread after timeout expiration.
 * 1. The MAIN part shows standard C program initialization with options for
 *    different logging capabilities of EVM.
 * 2. The EVM part demonstrates EVM initialization. 
*/

#ifndef hello4_evm_c
#define hello4_evm_c
#else
#error Preprocesor macro hello4_evm_c conflict!
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <evm/libevm.h>
#include "hello4_evm.h"

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
unsigned int num_additional_threads = 0;

static void usage_help(char *argv[])
{
	printf("Usage:\n");
	printf("\t%s [options] [num_additional_threads]\n", argv[0]);
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
		char *param = argv[optind];

		if ((num_additional_threads = atoi(param)) >= 0)
			optind++;
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
	evm_init_struct *evm_init_ptr;

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

	if ((evm_init_ptr = hello4_evm_init(1)) == NULL)
		exit(EXIT_FAILURE);

	if (hello4_evm_run(evm_init_ptr) < 0)
		exit(EXIT_FAILURE);

	exit(EXIT_SUCCESS);
}

/*
 * The EVM part.
 */

/*
 * Signal post-processing callback - optional for evm_init():
 * Covers SIGHUP and SIGCHLD
 */
static evm_sigpost_struct evs_sigpost = {
	.sigpost_handle = signal_processing
};

static int signal_processing(int sig, void *ptr)
{
	evm_log_info("(entry) sig=%d, ptr=%p\n", sig, ptr);
	return 0;
}

/*
 * Message event types table - required by evm_init():
 * Per type message id limits and parser callback
 */
static evm_msgs_link_struct evs_msgs_linkage[] = {
	[EV_TYPE_HELLO_MSG].first_ev_id = EV_ID_HELLO_MSG_HELLO,
	[EV_TYPE_HELLO_MSG].last_ev_id = EV_ID_HELLO_MSG_HELLO,
};

/*
 * Timer event types table - required by evm_init():
 * Per type timer id limits and parser callback
 */
static evm_tmrs_link_struct evs_tmrs_linkage[] = {
	[EV_TYPE_HELLO_TMR].first_ev_id = EV_ID_HELLO_TMR_IDLE,
	[EV_TYPE_HELLO_TMR].last_ev_id = EV_ID_HELLO_TMR_QUIT,
};

/*
 * Events table - required by evm_init():
 * Messages - their individual IDs and callbacks
 */
static evm_tab_struct evm_msgs_tbl[] = {
	{ /*HELLO messages*/
		.ev_type = EV_TYPE_HELLO_MSG,
		.ev_id = EV_ID_HELLO_MSG_HELLO,
		.ev_prepare = NULL,
		.ev_handle = evHelloMsg,
		.ev_finalize = evHelloMsgFree, /*free handled message*/
	}, { /*EOT - (End Of Table)*/
		.ev_handle = NULL,
	},
};

/*
 * Events table - required by evm_init():
 * Timers - their individual IDs and callbacks
 */
static evm_tab_struct evm_tmrs_tbl[] = {
	{ /*HELLO timers*/
		.ev_type = EV_TYPE_HELLO_TMR,
		.ev_id = EV_ID_HELLO_TMR_IDLE,
		.ev_handle = evHelloTmrIdle,
		.ev_finalize = evm_timer_finalize, /*internal freeing*/
	}, {
		.ev_type = EV_TYPE_HELLO_TMR,
		.ev_id = EV_ID_HELLO_TMR_QUIT,
		.ev_handle = evHelloTmrQuit,
		.ev_finalize = evm_timer_finalize, /*internal freeing*/
	}, { /*EOT - (End Of Table)*/
		.ev_handle = NULL,
	},
};

/* HELLO messages */
static char send_buff[MAX_BUFF_SIZE] = "";
static char recv_buff[MAX_BUFF_SIZE] = "";
static char *hello_str = "HELLO";

/* HELLO timers */
static evm_timer_struct *helloIdleTmr;
static evm_timer_struct *helloQuitTmr;

static evm_timer_struct * hello_start_timer(evm_init_struct *evm_ptr, evm_timer_struct *tmr, time_t tv_sec, long tv_nsec, void *ctx_ptr, int tmr_type, int tmr_id)
{
	evm_timer_struct *new = &evm_ptr->evm_tmrs_link[tmr_type].tmrs_tab[tmr_id];

	evm_log_info("(entry) tmr=%p, sec=%ld, nsec=%ld, ctx_ptr=%p\n", tmr, tv_sec, tv_nsec, ctx_ptr);
	evm_timer_stop(tmr);
	return evm_timer_start(evm_ptr, new->tmr_ids, tv_sec, tv_nsec, ctx_ptr);
}

/* HELLO event handlers */
static int evHelloMsg(void *ev_ptr)
{
	evm_message_struct *msg = (evm_message_struct *)ev_ptr;
	evm_init_struct *loc_evm_ptr = msg->evm_ptr;
	evm_init_struct *rem_evm_ptr = (evm_init_struct *)msg->ctx_ptr;

	evm_log_info("(cb entry) ev_ptr=%p\n", ev_ptr);
#if 1
	evm_log_notice("HELLO msg received: \"%s\"\n", (char *)msg->iov_buff.iov_base);

	evm_log_notice("IDLE timer set: 10 s\n");
	helloIdleTmr = hello_start_timer(loc_evm_ptr, NULL, 10, 0, msg->ctx_ptr, EV_TYPE_HELLO_TMR, EV_ID_HELLO_TMR_IDLE);
#else
	/* liveloop - 100 %CPU usage */
	/* Send HELLO message to another thread. */
//	evm_log_notice("HELLO msg sent: \"%s%d\"\n", "HELLO: ", *(int *)loc_evm_ptr->priv + 1);
	hello4_send_hello(loc_evm_ptr, rem_evm_ptr);
#endif

	return 0;
}

static int evHelloMsgFree(void *ev_ptr)
{
	evm_message_struct *msg = (evm_message_struct *)ev_ptr;

	evm_log_info("(cb entry) ev_ptr=%p\n", ev_ptr);

	free(msg->iov_buff.iov_base);
	msg->iov_buff.iov_base = NULL;
	free(msg);
	msg = NULL;

	return 0;
}

static int evHelloTmrIdle(void *ev_ptr)
{
	int status = 0;
	evm_init_struct *loc_evm_ptr = ((evm_timer_struct *)ev_ptr)->evm_ptr;
	evm_init_struct *rem_evm_ptr = (evm_init_struct *)((evm_timer_struct *)ev_ptr)->ctx_ptr;

	evm_log_info("(cb entry) ev_ptr=%p\n", ev_ptr);
	evm_log_notice("IDLE timer expired!\n");

	evm_log_notice("HELLO msg sent: \"%s%d\"\n", "HELLO: ", *(int *)loc_evm_ptr->priv + 1);
	hello4_send_hello(loc_evm_ptr, rem_evm_ptr);

	return status;
}

static int evHelloTmrQuit(void *ev_ptr)
{
	int status = 0;
	evm_timer_struct *tmr = (evm_timer_struct *)ev_ptr;
	int *count = (int*)tmr->evm_ptr->priv;

	evm_log_info("(cb entry) ev_ptr=%p\n", ev_ptr);
	//evm_log_notice("QUIT timer expired (%d messages sent)!\n", (*count) * 2);
	printf("QUIT timer expired (%d messages sent)!\n", (*count) * 2);

	exit(EXIT_SUCCESS);
}

static int hello4_send_hello(evm_init_struct *loc_evm_ptr, evm_init_struct *rem_evm_ptr)
{
	evm_message_struct *msg;
	int *count = (int*)loc_evm_ptr->priv;

	evm_log_info("(entry) loc_evm_ptr=%p, rem_evm_ptr=%p\n", loc_evm_ptr, rem_evm_ptr);

	if ((msg = (evm_message_struct *)calloc(1, sizeof(evm_message_struct))) == NULL)
		return -1;

	memcpy((void *)msg, (void *)&loc_evm_ptr->evm_msgs_link[EV_TYPE_HELLO_MSG].msgs_tab[EV_ID_HELLO_MSG_HELLO], sizeof(evm_message_struct));

	if ((msg->iov_buff.iov_base = calloc(MAX_BUFF_SIZE, sizeof(char))) == NULL)
		return -1;

	msg->ctx_ptr = (void *)loc_evm_ptr;
	/* Send HELLO message to another thread. */
	(*count)++;
	sprintf((char *)msg->iov_buff.iov_base, "%s: %u", hello_str, *count);
	evm_message_pass(rem_evm_ptr, msg);

	return 0;
}

/* EVM initialization */
static evm_init_struct * hello4_evm_init(int relink)
{
	evm_init_struct *ptr;
	int status = 0;
	int i;

	evm_log_info("(entry)\n");

	/* Setup EVM init structure. */
	if ((ptr = (evm_init_struct *)calloc(1, sizeof(evm_init_struct))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): internal message queue\n");
		return NULL;
	}
	evm_log_info("(TEST)\n");
	/* Initialize event machine for each thread... */
	ptr->priv = (void *)NULL;
	ptr->evm_sigpost = &evs_sigpost;
	ptr->evm_relink = relink;
	ptr->evm_msgs_link = evs_msgs_linkage;
	ptr->evm_tmrs_link = evs_tmrs_linkage;
	ptr->evm_msgs_link_max = sizeof(evs_msgs_linkage) / sizeof(evm_msgs_link_struct) - 1;
	ptr->evm_tmrs_link_max = sizeof(evs_tmrs_linkage) / sizeof(evm_tmrs_link_struct) - 1;
	ptr->evm_msgs_tab = evm_msgs_tbl;
	ptr->evm_tmrs_tab = evm_tmrs_tbl;
	ptr->epoll_max_events = MAX_EPOLL_EVENTS_PER_RUN;
	evm_log_debug("evs_msgs_linkage index size = %d\n", ptr->evm_msgs_link_max);
	evm_log_debug("evs_tmrs_linkage index size = %d\n", ptr->evm_tmrs_link_max);
	if ((status = evm_init(ptr)) < 0) {
		return NULL;
	}
	evm_log_debug("evm epoll FD is %d\n", ptr->epollfd);

	evm_log_info("(exit)\n");
	return ptr;
}

/* Main core processing (event loop) */
static void * hello4_new_thread_start(void *arg)
{
	evm_init_struct *evm_init_ptr;
	int count = 0;

	evm_log_info("(entry)\n");

	if ((evm_init_ptr = hello4_evm_init(0)) == NULL)
		return NULL;

	evm_init_ptr->priv = (void *)&count;
	/* Send initial HELLO to the first thread! */
	evm_log_notice("HELLO msg sent: \"%s%d\"\n", "HELLO", count + 1);
	hello4_send_hello(evm_init_ptr, (evm_init_struct *)arg);

	/*
	 * Main EVM processing (event loop)
	 */
	evm_run(evm_init_ptr);
	return NULL;
}

static int hello4_evm_run(evm_init_struct *ptr)
{
	int status = 0;
	int count = 0;
	int i;
	pthread_attr_t attr;
	pthread_t second_thread;

	evm_log_info("(entry)\n");

	if ((status = pthread_attr_init(&attr)) != 0)
		evm_log_return_system_err("pthread_attr_init()\n");

	for (i = 0; i < (num_additional_threads); i++) {
		if ((status = pthread_create(&second_thread, &attr, hello4_new_thread_start, (void *)ptr)) != 0)
			evm_log_return_system_err("pthread_create()\n");
	}

	ptr->priv = (void *)&count;
	/* Set initial QUIT timer */
	helloQuitTmr = hello_start_timer(ptr, NULL, 60, 0, NULL, EV_TYPE_HELLO_TMR, EV_ID_HELLO_TMR_QUIT);
	printf("QUIT timer set: 60 s\n");

	/*
	 * Main EVM processing (event loop)
	 */
	return evm_run(ptr);
}

