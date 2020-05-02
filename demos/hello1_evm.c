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

#define U2UP_LOG_NAME DEMO1EVM
#include <u2up-log/u2up-log.h>
/* Declare all other used "u2up-log" modules: */
U2UP_LOG_DECLARE(EVM_CORE);
U2UP_LOG_DECLARE(EVM_MSGS);
U2UP_LOG_DECLARE(EVM_TMRS);

enum evm_consumer_ids {
	EVM_CONSUMER_ID_0 = 0
};

enum evm_msgtype_ids {
	EV_TYPE_UNKNOWN_MSG = 0,
	EV_TYPE_HELLO_MSG
};

enum evm_msg_ids {
	EV_ID_HELLO_MSG_HELLO = 0
};

enum evm_tmr_ids {
	EV_ID_HELLO_TMR_IDLE = 0,
	EV_ID_HELLO_TMR_QUIT
};

static evmTimerStruct * hello_start_timer(evmTimerStruct *tmr, time_t tv_sec, long tv_nsec, void *ctx_ptr, evmTmridStruct *tmrid_ptr);

static int evHelloMsg(evmConsumerStruct *consumer, evmMessageStruct *msg);
static int evHelloTmrIdle(evmConsumerStruct *consumer, evmTimerStruct *tmr);
static int evHelloTmrQuit(evmConsumerStruct *consumer, evmTimerStruct *tmr);

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
unsigned int demo_liveloop = 0;

static void usage_help(char *argv[])
{
	printf("Usage:\n");
	printf("\t%s [options]\n", argv[0]);
	printf("options:\n");
	printf("\t-q, --quiet              Disable all output.\n");
	printf("\t-v, --verbose            Enable verbose output.\n");
	printf("\t-l, --liveloop           Enable liveloop measurement mode.\n");
#if (U2UP_LOG_MODULE_TRACE != 0)
	printf("\t-t, --trace              Enable trace output.\n");
#endif
#if (U2UP_LOG_MODULE_DEBUG != 0)
	printf("\t-g, --debug              Enable debug output.\n");
#endif
	printf("\t-s, --syslog             Enable syslog output (instead of stdout, stderr).\n");
	printf("\t-n, --no-header          No EVMLOG header added to every u2up_log_... output.\n");
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
			{"liveloop", 0, 0, 'l'},
#if (U2UP_LOG_MODULE_TRACE != 0)
			{"trace", 0, 0, 't'},
#endif
#if (U2UP_LOG_MODULE_DEBUG != 0)
			{"debug", 0, 0, 'g'},
#endif
			{"no-header", 0, 0, 'n'},
			{"syslog", 0, 0, 's'},
			{"help", 0, 0, 'h'},
			{0, 0, 0, 0}
		};

#if (U2UP_LOG_MODULE_TRACE != 0) && (U2UP_LOG_MODULE_DEBUG != 0)
		c = getopt_long(argc, argv, "qvltgnsh", long_options, &option_index);
#elif (U2UP_LOG_MODULE_TRACE == 0) && (U2UP_LOG_MODULE_DEBUG != 0)
		c = getopt_long(argc, argv, "qvlgnsh", long_options, &option_index);
#elif (U2UP_LOG_MODULE_TRACE != 0) && (U2UP_LOG_MODULE_DEBUG == 0)
		c = getopt_long(argc, argv, "qvltnsh", long_options, &option_index);
#else
		c = getopt_long(argc, argv, "qvlnsh", long_options, &option_index);
#endif
		if (c == -1)
			break;

		switch (c) {
		case 'q':
			U2UP_LOG_SET_NORMAL(0);
			U2UP_LOG_SET_NORMAL2(EVM_CORE, 0);
			U2UP_LOG_SET_NORMAL2(EVM_MSGS, 0);
			U2UP_LOG_SET_NORMAL2(EVM_TMRS, 0);
			evmlog_normal = 0;
			break;

		case 'v':
			U2UP_LOG_SET_VERBOSE(1);
			U2UP_LOG_SET_VERBOSE2(EVM_CORE, 1);
			U2UP_LOG_SET_VERBOSE2(EVM_MSGS, 1);
			U2UP_LOG_SET_VERBOSE2(EVM_TMRS, 1);
			evmlog_verbose = 1;
			break;

		case 'l':
			demo_liveloop = 1;
			break;

#if (U2UP_LOG_MODULE_TRACE != 0)
		case 't':
			U2UP_LOG_SET_TRACE(1);
			U2UP_LOG_SET_TRACE2(EVM_CORE, 1);
			U2UP_LOG_SET_TRACE2(EVM_MSGS, 1);
			U2UP_LOG_SET_TRACE2(EVM_TMRS, 1);
			evmlog_trace = 1;
			break;
#endif

#if (U2UP_LOG_MODULE_DEBUG != 0)
		case 'g':
			U2UP_LOG_SET_DEBUG(1);
			U2UP_LOG_SET_DEBUG2(EVM_CORE, 1);
			U2UP_LOG_SET_DEBUG2(EVM_MSGS, 1);
			U2UP_LOG_SET_DEBUG2(EVM_TMRS, 1);
			evmlog_debug = 1;
			break;
#endif

		case 'n':
			U2UP_LOG_SET_HEADER(0);
			U2UP_LOG_SET_HEADER2(EVM_CORE, 0);
			U2UP_LOG_SET_HEADER2(EVM_MSGS, 0);
			U2UP_LOG_SET_HEADER2(EVM_TMRS, 0);
			evmlog_add_header = 0;
			break;

		case 's':
			U2UP_LOG_SET_SYSLOG(1);
			U2UP_LOG_SET_SYSLOG2(EVM_CORE, 1);
			U2UP_LOG_SET_SYSLOG2(EVM_MSGS, 1);
			U2UP_LOG_SET_SYSLOG2(EVM_TMRS, 1);
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
 * General EVM structure - Provided by evm_init():
 */
static evmStruct *evm;
static evmConsumerStruct *consumer;

/* HELLO messages */
static char *hello_str = "HELLO";
static char msg_buff[MAX_BUFF_SIZE];
static struct iovec *iov_buff = NULL;
evmMessageStruct *helloMsg;

/* HELLO timers */
static evmTimerStruct *helloIdleTmr;
static evmTimerStruct *helloQuitTmr;

static evmTimerStruct * hello_start_timer(evmTimerStruct *tmr, time_t tv_sec, long tv_nsec, void *ctx_ptr, evmTmridStruct *tmrid_ptr)
{
	u2up_log_info("(entry) tmr=%p, sec=%ld, nsec=%ld, ctx_ptr=%p\n", tmr, tv_sec, tv_nsec, ctx_ptr);
	evm_timer_stop(tmr);
	return evm_timer_start(consumer, tmrid_ptr, tv_sec, tv_nsec, ctx_ptr);
}

static unsigned int count;

/* HELLO event handlers */
static int evHelloMsg(evmConsumerStruct *consumer, evmMessageStruct *msg)
{
	evmTmridStruct *tmrid_ptr;
	struct iovec *iov_buff = NULL;
	u2up_log_info("(cb entry) msg_ptr=%p\n", msg);

	if (msg == NULL)
		return -1;

	if (demo_liveloop == 0) {
		if ((iov_buff = (struct iovec *)evm_message_data_get(msg)) == NULL)
			return -1;
		u2up_log_notice("HELLO msg received: \"%s\"\n", (char *)iov_buff->iov_base);

		if ((tmrid_ptr = evm_tmrid_get(evm, EV_ID_HELLO_TMR_IDLE)) == NULL)
			return -1;
		helloIdleTmr = hello_start_timer(helloIdleTmr, 10, 0, NULL, tmrid_ptr);
		u2up_log_notice("IDLE timer set: 10 s\n");
	} else {
		count++;
		/* liveloop - 100 %CPU usage */
		evm_message_pass(consumer, msg);
	}

	return 0;
}

static int evHelloTmrIdle(evmConsumerStruct *consumer, evmTimerStruct *tmr)
{
	int rv = 0;
	u2up_log_info("(cb entry) tmr=%p\n", tmr);

	u2up_log_notice("IDLE timer expired!\n");

	count++;
	sprintf((char *)iov_buff->iov_base, "%s: %u", hello_str, count);
	evm_message_pass(consumer, helloMsg);
	u2up_log_notice("HELLO msg sent: \"%s\"\n", (char *)iov_buff->iov_base);

	return rv;
}

static int evHelloTmrQuit(evmConsumerStruct *consumer, evmTimerStruct *tmr)
{
	u2up_log_info("(cb entry) tmr=%p\n", tmr);

	u2up_log_notice("QUIT timer expired (%d messages sent)!\n", count);

	exit(EXIT_SUCCESS);
}

/* EVM initialization */
static int hello_evm_init(void)
{
	int rv = 0;
	evmMsgtypeStruct *msgtype_ptr;
	evmMsgidStruct *msgid_ptr;

	u2up_log_info("(entry)\n");

	/* Initialize event machine... */
	if ((evm = evm_init()) != NULL) {
		if ((rv == 0) && ((consumer = evm_consumer_add(evm, EVM_CONSUMER_ID_0)) == NULL)) {
			u2up_log_error("evm_consumer_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgtype_ptr = evm_msgtype_add(evm, EV_TYPE_HELLO_MSG)) == NULL)) {
			u2up_log_error("evm_msgtype_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_ptr = evm_msgid_add(msgtype_ptr, EV_ID_HELLO_MSG_HELLO)) == NULL)) {
			u2up_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && (evm_msgid_cb_handle_set(msgid_ptr, evHelloMsg) < 0)) {
			u2up_log_error("evm_msgid_cb_handle() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((helloMsg = evm_message_new(msgtype_ptr, msgid_ptr, sizeof(struct iovec))) == NULL)) {
			u2up_log_error("evm_message_new() failed!\n");
			rv = -1;
		}
		if (rv == 0) {
			evm_message_persistent_set(helloMsg);
			if ((iov_buff = (struct iovec *)evm_message_data_get(helloMsg)) == NULL)
				rv = -1;
			else
				iov_buff->iov_base = msg_buff;
		}
	} else {
		u2up_log_error("evm_init() failed!\n");
		rv = -1;
	}

	return rv;
}

/* Main core processing (event loop) */
static int hello_evm_run(void)
{
	evmTmridStruct *tmrid_ptr;

	/* Set initial IDLE timer */
	if ((tmrid_ptr = evm_tmrid_add(evm, EV_ID_HELLO_TMR_IDLE)) == NULL)
		return -1;
	if (evm_tmrid_cb_handle_set(tmrid_ptr, evHelloTmrIdle) < 0)
		return -1;
	helloIdleTmr = hello_start_timer(NULL, 0, 0, NULL, tmrid_ptr);
	u2up_log_notice("IDLE timer set: 0 s\n");

	/* Set initial QUIT timer */
	if ((tmrid_ptr = evm_tmrid_add(evm, EV_ID_HELLO_TMR_QUIT)) == NULL)
		return -1;
	if (evm_tmrid_cb_handle_set(tmrid_ptr, evHelloTmrQuit) < 0)
		return -1;
	helloQuitTmr = hello_start_timer(NULL, 60, 0, NULL, tmrid_ptr);
	u2up_log_notice("QUIT timer set: 60 s\n");

	/*
	 * Main EVM processing (event loop)
	 */
#if 1 /*orig*/
	return evm_run(consumer);
#else
	while (1) {
		evm_run_async(consumer);
		u2up_log_notice("Returned from evm_run_async()\n");
/**/		sleep(15);
		u2up_log_notice("Returned from sleep()\n");
/**/		sleep(2);
	}
	return 1;
#endif
}

