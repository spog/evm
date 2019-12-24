/*
 * The hello1_evm demo program
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
 * This demo shows minimal event machine usage within a single process. It sends
 * HELLO message to itself on every timeout expiration.
 * 1. The MAIN part shows standard C program initialization with options for
 *    different logging capabilities of EVM.
 * 2. The EVM part demonstrates EVM initialization. 
*/

#ifndef EVM_FILE_hello1_evm_c
#define EVM_FILE_hello1_evm_c
#else
#error Preprocesor macro EVM_FILE_hello1_evm_c conflict!
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <evm/libevm.h>
#include "hello1_evm.h"

#include <userlog/log_module.h>
EVMLOG_MODULE_INIT(DEMO1EVM, 2);

#define MAX_EPOLL_EVENTS_PER_RUN 10

static int signal_processing(int sig, void *ptr);

enum event_msg_types {
	EV_TYPE_UNKNOWN_MSG = 0,
	EV_TYPE_HELLO_MSG
};

enum event_tmr_types {
	EV_TYPE_UNKNOWN_TMR = 0,
	EV_TYPE_HELLO_TMR
};

enum hello_msg_ev_ids {
	EV_ID_HELLO_MSG_HELLO = 0
};
enum hello_tmr_ev_ids {
	EV_ID_HELLO_TMR_IDLE = 0,
	EV_ID_HELLO_TMR_QUIT
};

static evm_timer_struct * hello_start_timer(evm_timer_struct *tmr, time_t tv_sec, long tv_nsec, void *ctx_ptr, evm_evids_list_struct *tmrid_ptr);

static int evHelloMsg(void *ev_ptr);
static int evHelloTmrIdle(void *ev_ptr);
static int evHelloTmrQuit(void *ev_ptr);

static int hello_evm_init(void);
static int hello_evm_run(void);

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

/*
 * General EVM structure - required by evm_init() and evm_run():
 */
static evm_init_struct evs_init;

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

/* HELLO messages */
static char msg_buff[MAX_BUFF_SIZE];
static char *hello_str = "HELLO";
evm_message_struct helloMsg;

/* HELLO timers */
static evm_timer_struct *helloIdleTmr;
static evm_timer_struct *helloQuitTmr;

static evm_timer_struct * hello_start_timer(evm_timer_struct *tmr, time_t tv_sec, long tv_nsec, void *ctx_ptr, evm_evids_list_struct *tmrid_ptr)
{
	evm_log_info("(entry) tmr=%p, sec=%ld, nsec=%ld, ctx_ptr=%p\n", tmr, tv_sec, tv_nsec, ctx_ptr);
	evm_timer_stop(tmr);
	return evm_timer_start(&evs_init, tmrid_ptr, tv_sec, tv_nsec, ctx_ptr);
}

static unsigned int count;

/* HELLO event handlers */
static int evHelloMsg(void *ev_ptr)
{
	evm_evids_list_struct *tmrid_ptr;
	evm_message_struct *msg = (evm_message_struct *)ev_ptr;
#if 1
	evm_log_info("(cb entry) ev_ptr=%p\n", ev_ptr);
	evm_log_notice("HELLO msg received: \"%s\"\n", (char *)msg->iov_buff.iov_base);

	if ((tmrid_ptr = evm_tmrid_get(&evs_init, EV_ID_HELLO_TMR_IDLE)) == NULL)
		return -1;
	helloIdleTmr = hello_start_timer(helloIdleTmr, 10, 0, NULL, tmrid_ptr);
	evm_log_notice("IDLE timer set: 10 s\n");
#else
	count++;
	/* liveloop - 100 %CPU usage */
#if 1
	evm_message_call(&evs_init, &helloMsg);
#else
	evm_message_pass(&evs_init, &helloMsg);
#endif
#endif

	return 0;
}

static int evHelloTmrIdle(void *ev_ptr)
{
	int status = 0;
	evm_timer_struct *tmr = (evm_timer_struct *)ev_ptr;

	evm_log_info("(cb entry) ev_ptr=%p\n", ev_ptr);
	evm_log_notice("IDLE timer expired!\n");

	count++;
	sprintf((char *)helloMsg.iov_buff.iov_base, "%s: %u", hello_str, count);
	evm_message_call(tmr->evm_ptr, &helloMsg);
	evm_log_notice("HELLO msg sent: \"%s\"\n", (char *)helloMsg.iov_buff.iov_base);

	return status;
}

static int evHelloTmrQuit(void *ev_ptr)
{
	int status = 0;
	evm_timer_struct *tmr = (evm_timer_struct *)ev_ptr;

	evm_log_info("(cb entry) ev_ptr=%p\n", ev_ptr);
	evm_log_notice("QUIT timer expired (%d messages sent)!\n", count);

	exit(EXIT_SUCCESS);
}

/* EVM initialization */
static int hello_evm_init(void)
{
	int status = 0;
	evm_evtypes_list_struct *msgtype_ptr;
	evm_evids_list_struct *msgid_ptr;

	evm_log_info("(entry)\n");

	/* Initialize event machine... */
	evs_init.evm_sigpost = &evs_sigpost;;
	evs_init.epoll_max_events = MAX_EPOLL_EVENTS_PER_RUN;
	evs_init.epoll_timeout = -1; /* -1: wait indefinitely | 0: do not wait (asynchronous operation) */
	if ((status = evm_init(&evs_init)) < 0) {
		evm_log_error("evm_init() failed!\n");
		return status;
	}
	evm_log_debug("evm epoll FD is %d\n", evs_init.epollfd);

	if ((msgtype_ptr = evm_msgtype_add(&evs_init, EV_TYPE_HELLO_MSG)) == NULL)
		return -1;
	if ((msgid_ptr = evm_msgid_add(msgtype_ptr, EV_ID_HELLO_MSG_HELLO)) == NULL)
		return -1;
	if (evm_msgid_handle_cb_set(msgid_ptr, evHelloMsg) < 0)
		return -1;
	helloMsg.msgtype_ptr = msgtype_ptr;
	helloMsg.msgid_ptr = msgid_ptr;
	helloMsg.iov_buff.iov_base = (void *)msg_buff;

	return 0;
}

/* Main core processing (event loop) */
static int hello_evm_run(void)
{
	evm_evids_list_struct *tmrid_ptr;

	/* Set initial IDLE timer */
	if ((tmrid_ptr = evm_tmrid_add(&evs_init, EV_ID_HELLO_TMR_IDLE)) == NULL)
		return -1;
	if (evm_tmrid_handle_cb_set(tmrid_ptr, evHelloTmrIdle) < 0)
		return -1;
	helloIdleTmr = hello_start_timer(NULL, 0, 0, NULL, tmrid_ptr);
	evm_log_notice("IDLE timer set: 0 s\n");

	/* Set initial QUIT timer */
	if ((tmrid_ptr = evm_tmrid_add(&evs_init, EV_ID_HELLO_TMR_QUIT)) == NULL)
		return -1;
	if (evm_tmrid_handle_cb_set(tmrid_ptr, evHelloTmrQuit) < 0)
		return -1;
	helloQuitTmr = hello_start_timer(NULL, 60, 0, NULL, tmrid_ptr);
	evm_log_notice("QUIT timer set: 60 s\n");

	/*
	 * Main EVM processing (event loop)
	 */
#if 1 /*orig*/
	return evm_run(&evs_init);
#else
	while (1) {
		evm_run_async(&evs_init);
		evm_log_notice("Returned from evm_run_async()\n");
/**/		sleep(15);
		evm_log_notice("Returned from sleep()\n");
/**/		sleep(2);
	}
	return 1;
#endif
}

