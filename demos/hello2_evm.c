/*
 * The hello2_evm demo program
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
 * This demo shows sending message between two processes. The parent process
 * sends the first HELLO message to its child process. Every received
 * message in a child or parent process sets new timeout and another HELLO
 * message is sent back to the sender process after timeout expiration.
 * 1. The MAIN part shows standard C program initialization with options for
 *    different logging capabilities of EVM.
 * 2. The EVM part demonstrates EVM initialization. 
*/

#ifndef EVM_FILE_hello2_evm_c
#define EVM_FILE_hello2_evm_c
#else
#error Preprocesor macro EVM_FILE_hello2_evm_c conflict!
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
#include "hello2_evm.h"

#define U2UP_LOG_NAME DEMO2EVM
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

static evmTimerStruct * hello_start_timer(evmConsumerStruct *consumer_ptr, evmTimerStruct *tmr, time_t tv_sec, long tv_nsec, void *ctx_ptr, evmTmridStruct *tmrid_ptr);

static int hello2_fork_and_connect(void);
static int hello2_socket_send_hello(int sock);
static int hello2_receive(int sock);
static int hello2_parse(evmMessageStruct *msg);

static int evHelloMsg(evmConsumerStruct *consumer, evmMessageStruct *msg);
static int evHelloTmrIdle(evmConsumerStruct *consumer, evmTimerStruct *tmr);
static int evHelloTmrQuit(evmConsumerStruct *consumer, evmTimerStruct *tmr);

static int hello2_evm_init(void);
static int hello2_evm_run(void);

/*
 * The MAIN part.
 */
unsigned int log_mask;
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
	printf("\t-n, --no-header          No U2UP_LOG header added to every u2up_log_... output.\n");
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
			break;

		case 'v':
			U2UP_LOG_SET_VERBOSE(1);
			U2UP_LOG_SET_VERBOSE2(EVM_CORE, 1);
			U2UP_LOG_SET_VERBOSE2(EVM_MSGS, 1);
			U2UP_LOG_SET_VERBOSE2(EVM_TMRS, 1);
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
			break;
#endif

#if (U2UP_LOG_MODULE_DEBUG != 0)
		case 'g':
			U2UP_LOG_SET_DEBUG(1);
			U2UP_LOG_SET_DEBUG2(EVM_CORE, 1);
			U2UP_LOG_SET_DEBUG2(EVM_MSGS, 1);
			U2UP_LOG_SET_DEBUG2(EVM_TMRS, 1);
			break;
#endif

		case 'n':
			U2UP_LOG_SET_HEADER(0);
			U2UP_LOG_SET_HEADER2(EVM_CORE, 0);
			U2UP_LOG_SET_HEADER2(EVM_MSGS, 0);
			U2UP_LOG_SET_HEADER2(EVM_TMRS, 0);
			break;

		case 's':
			U2UP_LOG_SET_SYSLOG(1);
			U2UP_LOG_SET_SYSLOG2(EVM_CORE, 1);
			U2UP_LOG_SET_SYSLOG2(EVM_MSGS, 1);
			U2UP_LOG_SET_SYSLOG2(EVM_TMRS, 1);
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

static evmConsumerStruct *consumer;

int main(int argc, char *argv[])
{
	usage_check(argc, argv);

	log_mask = LOG_MASK(LOG_EMERG) | LOG_MASK(LOG_ALERT) | LOG_MASK(LOG_CRIT) | LOG_MASK(LOG_ERR);

	/* Setup LOG_MASK according to startup arguments! */
	if (U2UP_LOG_GET_NORMAL()) {
		log_mask |= LOG_MASK(LOG_WARNING);
		log_mask |= LOG_MASK(LOG_NOTICE);
	}
	if ((U2UP_LOG_GET_VERBOSE()) || (U2UP_LOG_GET_TRACE()))
		log_mask |= LOG_MASK(LOG_INFO);
	if (U2UP_LOG_GET_DEBUG())
		log_mask |= LOG_MASK(LOG_DEBUG);

	setlogmask(log_mask);

	if (hello2_evm_init() < 0)
		exit(EXIT_FAILURE);

	if (hello2_evm_run() < 0)
		exit(EXIT_FAILURE);

	exit(EXIT_SUCCESS);
}

/*
 * The EVM part.
 */
static int child = 0;
static int count = 0;
static int sock;

/* HELLO messages */
static char *hello_str = "HELLO";
evmMessageStruct *helloMsg;

/* HELLO timers */
static evmTimerStruct *helloIdleTmr;
static evmTimerStruct *helloQuitTmr;

static evmTimerStruct * hello_start_timer(evmConsumerStruct *consumer_ptr, evmTimerStruct *tmr, time_t tv_sec, long tv_nsec, void *ctx_ptr, evmTmridStruct *tmrid_ptr)
{
	u2up_log_info("(entry) tmr=%p, sec=%ld, nsec=%ld, ctx_ptr=%p\n", tmr, tv_sec, tv_nsec, ctx_ptr);
	evm_timer_stop(tmr);
	return evm_timer_start(consumer_ptr, tmrid_ptr, tv_sec, tv_nsec, ctx_ptr);
}

/* HELLO event handlers */
static int evHelloMsg(evmConsumerStruct *consumer, evmMessageStruct *msg)
{
	struct iovec *iov_buff = NULL;
	u2up_log_info("(cb entry) msg=%p\n", msg);

	if (msg == NULL)
		return -1;

	if (demo_liveloop == 0) {
		if ((iov_buff = (struct iovec *)evm_message_data_get(msg)) == NULL)
			return -1;
		u2up_log_notice("HELLO msg received: \"%s\"\n", (char *)iov_buff->iov_base);

		helloIdleTmr = hello_start_timer(consumer, NULL, 10, 0, NULL, tmrid_idle_ptr);
		u2up_log_notice("IDLE timer set: 10 s\n");
	} else {
		/* liveloop - 100 %CPU usage */
		/* Send HELLO message to another thread. */
		hello2_socket_send_hello(sock);
	}

	return 0;
}

static int evHelloTmrIdle(evmConsumerStruct *consumer, evmTimerStruct *tmr)
{
	int status = 0;

	u2up_log_info("(cb entry) tmr=%p\n", tmr);

	if (tmr == NULL)
		return -1;

	u2up_log_notice("IDLE timer expired!\n");

	hello2_socket_send_hello(sock);

	return status;
}

static int evHelloTmrQuit(evmConsumerStruct *consumer, evmTimerStruct *tmr)
{
	u2up_log_info("(cb entry) tmr=%p\n", tmr);

	if (tmr == NULL)
		return -1;

	u2up_log_notice("QUIT timer expired (%d messages sent)!\n", count);

	exit(EXIT_SUCCESS);
}

static int hello2_fork_and_connect(void)
{
	int sd[2];
	pid_t child_pid;

	u2up_log_info("(entry)\n");
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sd) == -1)
		u2up_log_return_system_err("socketpair()\n");

	child_pid = fork();
	switch (child_pid) {
	case -1: /*error*/
		u2up_log_return_system_err("fork()\n");
	case 0: /*child*/
		child = 1;
		close(sd[0]);
		return sd[1];
	}

	/*parent*/
	close(sd[1]);
	return sd[0];
}

static int hello2_socket_send_hello(int sock)
{
	struct iovec *iov_buff = NULL;
	struct msghdr msg;
	int err_save = EINVAL;
	int ret;
	u2up_log_info("(entry) sockfd=%d\n", sock);

	if ((iov_buff = (struct iovec *)evm_message_data_get(helloMsg)) == NULL) {
		return -1;
	}
	/* Prepare message buffer. */
	sprintf((char *)iov_buff->iov_base, "%s: %d", hello_str, ++count);
	iov_buff->iov_len = strlen(iov_buff->iov_base);
	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = iov_buff;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	/* Send HELLO message. */
	if (demo_liveloop == 0)
		u2up_log_notice("HELLO msg send: \"%s\"\n", (char *)iov_buff->iov_base);
	ret = sendmsg(sock, &msg, 0);
	if (ret == -1) {
		err_save = errno;
		if (err_save != EINTR) {
			u2up_log_system_error("sendmsg(): sockfd=%d\n", sock);
		}
		return -err_save;
	}

	u2up_log_debug("sent=%d\n", ret);
	return ret;
}

/*
 * Over symplified message parsing:)
 */
static int hello2_parse(evmMessageStruct *msg)
{
	int rv = 0;
	struct iovec *iov_buff = NULL;
	u2up_log_info("(entry) iov_buff=%p\n", iov_buff);

	iov_buff = (struct iovec*)evm_message_data_get(msg);
	if (iov_buff == NULL) {
		u2up_log_debug("Receive buffer is NULL.\n");
		return -1;
	}
	u2up_log_debug("iov_buff->iov_len=%zd\n", iov_buff->iov_len);
	if (iov_buff->iov_len == 0) {
		u2up_log_debug("No event decoded (empty receive buffer).\n");
		return -1;
	} else {
		/* Decode the INPUT buffer. */ 
		((char *)iov_buff->iov_base)[iov_buff->iov_len] = '\0';
		if (strncmp((char *)iov_buff->iov_base, hello_str, strlen(hello_str)) == 0) {
			sscanf((char *)iov_buff->iov_base, "HELLO: %d", &count);
			rv = evm_message_pass(consumer, msg);
		} else {
			u2up_log_debug("No event decoded (unknown data: %s).\n", (char *)iov_buff->iov_base);
			return -1;
		}
	}

	return rv;
}

static int hello2_receive(int sock)
{
	struct iovec *iov_buff = NULL;
	struct msghdr msg;
	evmMessageStruct *evm_msg;
	int err_save = EINVAL;
	int ret;
	u2up_log_info("(cb entry) sock=%d\n", sock);

	if ((evm_msg = evm_message_new(msgtype_hello_ptr, msgid_hello_ptr, sizeof(struct iovec))) == NULL) {
		u2up_log_error("evm_message_new() failed!\n");
		return -1;
	}
	if ((iov_buff = (struct iovec *)evm_message_data_get(evm_msg)) == NULL)
		return -1;
	else {
		if ((iov_buff->iov_base = calloc(MAX_BUFF_SIZE, sizeof(char))) == NULL)
			return -1;
	}
	if (evm_message_alloc_add(evm_msg, iov_buff->iov_base) != 0) {
		free(iov_buff);
		return -1;
	}

	/* Prepare receive buffer. */
	iov_buff->iov_len = MAX_BUFF_SIZE;
	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = iov_buff;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	/* Receive whatever message comes in. */
	ret = recvmsg(sock, &msg, 0);
	if (ret == -1) {
		err_save = errno;
		if (err_save != EINTR) {
			u2up_log_system_error("recvmsg(): sock=%d\n", sock);
		}
		return -err_save;
	}
	iov_buff->iov_len = ret;
	((char *)(iov_buff->iov_base))[ret] = '\0';

	u2up_log_debug("buffered=%zd, received data: %s\n", iov_buff->iov_len, (char *)iov_buff->iov_base);
	return hello2_parse(evm_msg);
}

/* EVM initialization */
static int hello2_evm_init(void)
{
	int rv = 0;
	struct iovec *iov_buff = NULL;

	u2up_log_info("(entry)\n");

	sock = hello2_fork_and_connect();

	/* Initialize event machine... */
	if ((evm = evm_init()) != NULL) {
		if ((rv == 0) && ((consumer = evm_consumer_add(evm, EVM_CONSUMER_ID_0)) == NULL)) {
			u2up_log_error("evm_consumer_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgtype_hello_ptr = evm_msgtype_add(evm, EV_TYPE_HELLO_MSG)) == NULL)) {
			u2up_log_error("evm_msgtype_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((msgid_hello_ptr = evm_msgid_add(msgtype_hello_ptr, EV_ID_HELLO_MSG_HELLO)) == NULL)) {
			u2up_log_error("evm_msgid_add() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && (evm_msgid_cb_handle_set(msgid_hello_ptr, evHelloMsg) < 0)) {
			u2up_log_error("evm_msgid_cb_handle_set() failed!\n");
			rv = -1;
		}
		if ((rv == 0) && ((helloMsg = evm_message_new(msgtype_hello_ptr, msgid_hello_ptr, sizeof(struct iovec))) == NULL)) {
			u2up_log_error("evm_message_new() failed!\n");
			rv = -1;
		}
		if (rv == 0) {
			evm_message_persistent_set(helloMsg);
			if ((iov_buff = (struct iovec *)evm_message_data_get(helloMsg)) == NULL)
				rv = -1;
			else {
				if ((iov_buff->iov_base = calloc(MAX_BUFF_SIZE, sizeof(char))) == NULL)
					return -1;
			}
		}
		if ((rv == 0) && ((tmrid_idle_ptr = evm_tmrid_add(evm, EV_ID_HELLO_TMR_IDLE)) == NULL)) {
			u2up_log_error("evm_tmrid_add() failed!\n");
			return -1;
		}
		if ((rv == 0) && (evm_tmrid_cb_handle_set(tmrid_idle_ptr, evHelloTmrIdle) < 0)) {
			u2up_log_error("evm_tmrid_cb_handle() failed!\n");
			return -1;
		}
		if ((rv == 0) && ((tmrid_quit_ptr = evm_tmrid_add(evm, EV_ID_HELLO_TMR_QUIT)) == NULL)) {
			u2up_log_error("evm_tmrid_add() failed!\n");
			return -1;
		}
		if ((rv == 0) && (evm_tmrid_cb_handle_set(tmrid_quit_ptr, evHelloTmrQuit) < 0)) {
			u2up_log_error("evm_tmrid_cb_handle() failed!\n");
			return -1;
		}
	} else {
		u2up_log_error("evm_init() failed!\n");
		rv = -1;
	}

	u2up_log_info("(exit)\n");
	return rv;
}

/* Socker receiver thread processing loop */
static void * hello2_socket_receiver_thread_start(void *arg)
{
	u2up_log_info("(entry)\n");

	while (1) {
		if (hello2_receive(sock) < 0)
			break;
	}
	return NULL;
}

/* Main core processing (event loop) */
static int hello2_evm_run(void)
{
	int rv = 0;
	pthread_attr_t attr;
	pthread_t socket_receiver_thread;
	u2up_log_info("(entry) child=%d\n", child);

	/* Send first HELLO to the child process! */
	if (!child) {
		hello2_socket_send_hello(sock);
	}

	/* Start initial QUIT timer */
	helloQuitTmr = hello_start_timer(consumer, NULL, 60, 0, NULL, tmrid_quit_ptr);
	u2up_log_notice("QUIT timer set: 60 s\n");

	if ((rv = pthread_attr_init(&attr)) != 0)
		u2up_log_return_system_err("pthread_attr_init()\n");

	if ((rv = pthread_create(&socket_receiver_thread, &attr, hello2_socket_receiver_thread_start, NULL)) != 0)
		u2up_log_return_system_err("pthread_create()\n");

	/*
	 * Main thread EVM processing (event loop)
	 */
	return evm_run(consumer);
}

