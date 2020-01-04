/*
 * The hello3_evm demo program
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
 * thread spawns the second thread and sets QUIT timer. The second thread
 * sends the first HELLO message to the first thread. Every received
 * message in any thread sets a new timeout and another HELLO message is
 * sent back to the sender thread after timeout expiration.
 * 1. The MAIN part shows standard C program initialization with options for
 *    different logging capabilities of EVM.
 * 2. The EVM part demonstrates EVM initialization. 
*/

#ifndef EVM_FILE_hello3_evm_c
#define EVM_FILE_hello3_evm_c
#else
#error Preprocesor macro EVM_FILE_hello3_evm_c conflict!
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
#include "hello3_evm.h"

#include <userlog/log_module.h>
EVMLOG_MODULE_INIT(DEMO3EVM, 2);

enum evm_consumer_ids {
	EVM_CONSUMER_ID_0 = 0
};

enum evm_msgtype_ids {
	EV_TYPE_UNKNOWN_MSG = 0,
	EV_TYPE_HELLO_MSG
};

enum evm_msg_ids {
	EV_ID_HELLO_MSG_UNKNOWN0 = 0,
	EV_ID_HELLO_MSG_UNKNOWN1,
	EV_ID_HELLO_MSG_UNKNOWN2,
	EV_ID_HELLO_MSG_UNKNOWN3,
	EV_ID_HELLO_MSG_UNKNOWN4,
	EV_ID_HELLO_MSG_UNKNOWN5,
	EV_ID_HELLO_MSG_UNKNOWN6,
	EV_ID_HELLO_MSG_UNKNOWN7,
	EV_ID_HELLO_MSG_UNKNOWN8,
	EV_ID_HELLO_MSG_UNKNOWN9,
	EV_ID_HELLO_MSG_UNKNOWN10,
	EV_ID_HELLO_MSG_UNKNOWN11,
	EV_ID_HELLO_MSG_UNKNOWN12,
	EV_ID_HELLO_MSG_UNKNOWN13,
	EV_ID_HELLO_MSG_UNKNOWN14,
	EV_ID_HELLO_MSG_UNKNOWN15,
	EV_ID_HELLO_MSG_UNKNOWN16,
	EV_ID_HELLO_MSG_UNKNOWN17,
	EV_ID_HELLO_MSG_UNKNOWN18,
	EV_ID_HELLO_MSG_HELLO,
	EV_NUM_IDS
};

enum evm_tmr_ids {
	EV_ID_HELLO_TMR_IDLE = 0,
	EV_ID_HELLO_TMR_QUIT
};

static evmTimerStruct * hello_start_timer(evmConsumerStruct *consumer_ptr, evmTimerStruct *tmr, time_t tv_sec, long tv_nsec, void *ctx_ptr, evmTmridStruct *tmrid_ptr);
static int hello3_send_hello(evmConsumerStruct *loc_evm_ptr, evmConsumerStruct *rem_evm_ptr);

static int evHelloMsg(void *msg_ptr);
static int evHelloTmrIdle(void *tmr_ptr);
static int evHelloTmrQuit(void *tmr_ptr);

static int hello3_evm_init(void);
static int hello3_evm_run(void);

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
			{"liveloop", 0, 0, 'l'},
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
		c = getopt_long(argc, argv, "qvltgnsh", long_options, &option_index);
#elif (EVMLOG_MODULE_TRACE == 0) && (EVMLOG_MODULE_DEBUG != 0)
		c = getopt_long(argc, argv, "qvlgnsh", long_options, &option_index);
#elif (EVMLOG_MODULE_TRACE != 0) && (EVMLOG_MODULE_DEBUG == 0)
		c = getopt_long(argc, argv, "qvltnsh", long_options, &option_index);
#else
		c = getopt_long(argc, argv, "qvlnsh", long_options, &option_index);
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

		case 'l':
			demo_liveloop = 1;
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

static evmStruct *evm;
static evmTmridStruct *tmrid_idle_ptr;
static evmTmridStruct *tmrid_quit_ptr;
static evmMsgidStruct *msgid_hello_ptr;
static evmMsgtypeStruct *msgtype_hello_ptr;

static evmConsumerStruct **consumers;

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

	if (hello3_evm_init() < 0)
		exit(EXIT_FAILURE);

	if (hello3_evm_run() < 0)
		exit(EXIT_FAILURE);

	exit(EXIT_SUCCESS);
}

/*
 * The EVM part.
 */
static int count = 0; /* common global counter shared beteen both threads */

/* HELLO messages */
static char *hello_str = "HELLO";
evmMessageStruct *helloMsg;

/* HELLO timers */
static evmTimerStruct *helloIdleTmr;
static evmTimerStruct *helloQuitTmr;

static evmTimerStruct * hello_start_timer(evmConsumerStruct *consumer_ptr, evmTimerStruct *tmr, time_t tv_sec, long tv_nsec, void *ctx_ptr, evmTmridStruct *tmrid_ptr)
{
	evm_log_info("(entry) tmr=%p, sec=%ld, nsec=%ld, ctx_ptr=%p\n", tmr, tv_sec, tv_nsec, ctx_ptr);
	evm_timer_stop(tmr);
	return evm_timer_start(consumer_ptr, tmrid_ptr, tv_sec, tv_nsec, ctx_ptr);
}

/* HELLO event handlers */
static int evHelloMsg(void *msg_ptr)
{
	struct iovec *iov_buff = NULL;
	evmMessageStruct *msg = (evmMessageStruct *)msg_ptr;
	evmConsumerStruct *loc_consumer_ptr;
	evmConsumerStruct *rem_consumer_ptr;
	evm_log_info("(cb entry) msg_ptr=%p\n", msg_ptr);

	if (msg == NULL)
		return -1;

	loc_consumer_ptr = evm_message_consumer_get(msg);
	rem_consumer_ptr = (evmConsumerStruct *)evm_message_ctx_get(msg);

	if (demo_liveloop == 0) {
		if ((iov_buff = evm_message_iovec_get(msg)) == NULL)
			return -1;
		evm_log_notice("HELLO msg received: \"%s\"\n", (char *)iov_buff->iov_base);

		helloIdleTmr = hello_start_timer(loc_consumer_ptr, NULL, 10, 0, (void *)rem_consumer_ptr, tmrid_idle_ptr);
		evm_log_notice("IDLE timer set: 10 s\n");
	} else {
		/* liveloop - 100 %CPU usage */
		/* Send HELLO message to another thread. */
		hello3_send_hello(loc_consumer_ptr, rem_consumer_ptr);
	}

	return 0;
}

static int evHelloTmrIdle(void *tmr_ptr)
{
	int status = 0;
	evmTimerStruct *tmr = (evmTimerStruct *)tmr_ptr;
	evmConsumerStruct *loc_consumer;
	evmConsumerStruct *rem_consumer;

	evm_log_info("(cb entry) tmr_ptr=%p\n", tmr_ptr);

	if (tmr == NULL)
		return -1;

	evm_log_notice("IDLE timer expired!\n");
	loc_consumer = evm_timer_consumer_get(tmr);
	rem_consumer = (evmConsumerStruct *)evm_timer_ctx_get(tmr);

	hello3_send_hello(loc_consumer, rem_consumer);

	return status;
}

static int evHelloTmrQuit(void *tmr_ptr)
{
	evmTimerStruct *tmr = (evmTimerStruct *)tmr_ptr;
	evm_log_info("(cb entry) tmr_ptr=%p\n", tmr_ptr);

	if (tmr == NULL)
		return -1;

	evm_log_notice("QUIT timer expired (%d messages sent)!\n", count);

	exit(EXIT_SUCCESS);
}

static int hello3_send_hello(evmConsumerStruct *loc_consumer_ptr, evmConsumerStruct *rem_consumer_ptr)
{
	struct iovec *iov_buff = NULL;

	evm_log_info("(entry) loc_consumer_ptr=%p, rem_consumer_ptr=%p\n", loc_consumer_ptr, rem_consumer_ptr);

	if ((iov_buff = evm_message_iovec_get(helloMsg)) == NULL) {
		return -1;
	}
	count++;
	sprintf((char *)iov_buff->iov_base, "%s: %u", hello_str, count);

	evm_message_ctx_set(helloMsg, (void *)loc_consumer_ptr);

	/* Send HELLO message to another thread. */
	if (demo_liveloop == 0)
		evm_log_notice("HELLO msg send: %s\n", (char *)iov_buff->iov_base);
	evm_message_pass(rem_consumer_ptr, helloMsg);

	return 0;
}

/* EVM initialization */
static int hello3_evm_init(void)
{
	int rv = 0;
	struct iovec *iov_buff = NULL;

	evm_log_info("(entry)\n");

	/* Prepare consumers table for 2 */
	if ((consumers = (evmConsumerStruct **)calloc(2, sizeof(evmConsumerStruct *))) == NULL)
		return -1;

	/* Initialize event machine... */
	if ((evm = evm_init()) != NULL) {
		if ((rv == 0) && ((consumers[0] = evm_consumer_add(evm, EVM_CONSUMER_ID_0)) == NULL)) {
			evm_log_error("evm_consumer_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgtype_hello_ptr = evm_msgtype_add(evm, EV_TYPE_HELLO_MSG)) == NULL)) {
			evm_log_error("evm_msgtype_add() failed!\n");
			rv = -1;
		}
#if 1
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_UNKNOWN0)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_UNKNOWN1)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_UNKNOWN2)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_UNKNOWN3)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_UNKNOWN4)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_UNKNOWN5)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_UNKNOWN6)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_UNKNOWN7)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_UNKNOWN8)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_UNKNOWN9)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_UNKNOWN10)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_UNKNOWN11)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_UNKNOWN12)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_UNKNOWN13)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_UNKNOWN14)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_UNKNOWN15)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_UNKNOWN16)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_UNKNOWN17)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_UNKNOWN18)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
#endif
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_HELLO)) == NULL)) {
			evm_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && (evm_msgid_cb_handle_set(msgid_hello_ptr, evHelloMsg) < 0)) {
			evm_log_error("evm_msgid_cb_handle() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((helloMsg = evm_message_new(msgtype_hello_ptr, msgid_hello_ptr)) == NULL)) {
			evm_log_error("evm_message_new() failed!\n");
			rv = -1;
		}
		if (rv == 0) {
			if ((iov_buff = (struct iovec *)calloc(1, sizeof(struct iovec))) == NULL)
				return -1;
			if ((iov_buff->iov_base = calloc(MAX_BUFF_SIZE, sizeof(char))) == NULL) {
				free(iov_buff);
				return -1;
			}
			rv = evm_message_iovec_set(helloMsg, iov_buff);
		}
		if ((rv == 0) && ((tmrid_idle_ptr = evm_tmrid_add(evm, EV_ID_HELLO_TMR_IDLE)) == NULL)) {
			evm_log_error("evm_tmrid_add() failed!\n");
			return -1;
		}
		if ((rv == 0) && (evm_tmrid_cb_handle_set(tmrid_idle_ptr, evHelloTmrIdle) < 0)) {
			evm_log_error("evm_tmrid_cb_handle() failed!\n");
			return -1;
		}
		if ((rv == 0) && ((tmrid_quit_ptr = evm_tmrid_add(evm, EV_ID_HELLO_TMR_QUIT)) == NULL)) {
			evm_log_error("evm_tmrid_add() failed!\n");
			return -1;
		}
		if ((rv == 0) && (evm_tmrid_cb_handle_set(tmrid_quit_ptr, evHelloTmrQuit) < 0)) {
			evm_log_error("evm_tmrid_cb_handle() failed!\n");
			return -1;
		}
	} else {
		evm_log_error("evm_init() failed!\n");
		rv = -1;
	}

	evm_log_info("(exit)\n");
	return rv;
}

/* Main core processing (event loop) */
static void * hello3_second_thread_start(void *arg)
{
	evmConsumerStruct *consumer_ptr;
	evm_log_info("(entry)\n");

	if (arg == NULL)
		return NULL;

	consumer_ptr = (evmConsumerStruct *)arg;
	evm_consumer_priv_set(consumer_ptr, (void *)&count);
	/* Send initial HELLO to the first thread! */
	hello3_send_hello(consumer_ptr, consumers[0]);

	/*
	 * Second thread EVM processing (event loop)
	 */
	evm_run(consumer_ptr);
	return NULL;
}

static int hello3_evm_run(void)
{
	int rv = 0;
	pthread_attr_t attr;
	pthread_t second_thread;
	evm_log_info("(entry)\n");

	evm_consumer_priv_set(consumers[0], (void *)&count);

	/* Start initial QUIT timer */
	helloQuitTmr = hello_start_timer(consumers[0], NULL, 60, 0, NULL, tmrid_quit_ptr);
	evm_log_notice("QUIT timer set: 60 s\n");

	if ((rv = pthread_attr_init(&attr)) != 0)
		evm_log_return_system_err("pthread_attr_init()\n");

	if ((rv == 0) && ((consumers[1] = evm_consumer_add(evm, 1)) == NULL)) {
		evm_log_error("evm_consumer_add() failed!\n");
		rv = -1;
	}
	if ((rv = pthread_create(&second_thread, &attr, hello3_second_thread_start, (void *)consumers[1])) != 0)
		evm_log_return_system_err("pthread_create()\n");

	/*
	 * Main thread EVM processing (event loop)
	 */
	return evm_run(consumers[0]);
}

