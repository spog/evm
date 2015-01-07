/*
 * The hello2_evm demo program
 *
 * Copyright (C) 2014 Samo Pogacnik <samo_pogacnik@t-2.net>
 * All rights reserved.
 *
 * This file is part of the EVM software project.
 * This file is provided under the terms of the BSD 3-Clause license,
 * available in the LICENSE file of the EVM software project.
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

#ifndef hello2_evm_c
#define hello2_evm_c
#else
#error Preprocesor macro hello2_evm_c conflict!
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <evm/libevm.h>
#include "hello2_evm.h"

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
static int hello_count = 0;
static int sock;

/*
 * General EVM structure - required by evm_init() and evm_run():
 */
static evm_init_struct evs_init;

/*
 * File descriptors structure - required by evm_fd_add():
 */
static evm_fd_struct evs_fd;

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
 * Event types table - required by evm_init():
 * Per event type parser and linkage callbacks
 */
static evm_link_struct evs_linkage[] = {
	[EV_TYPE_HELLO_MSG].ev_type_parse = hello2_parse_message,
	[EV_TYPE_HELLO_MSG].ev_type_link = hello_messages_link,
	[EV_TYPE_HELLO_TMR].ev_type_link = helloTmrs_link,
};

/*
 * Events table - required by evm_init():
 * Messages and timers - their individual IDs and callbacks
 */
static evm_tab_struct evm_tbl[] = {
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
		.ev_finalize = evm_timer_finalize, /*internal freeing*/
	}, { /*EOT - (End Of Table)*/
		.ev_handle = NULL,
	},
};

/* HELLO messages */
static char send_buff[MAX_BUFF_SIZE] = "";
static char recv_buff[MAX_BUFF_SIZE] = "";
static char *hello_str = "HELLO";
static evm_message_struct helloMsg = {
	.msg_ids.ev_id = EV_ID_HELLO_MSG_HELLO,
	.iov_buff.iov_base = (void *)send_buff,
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
static evm_timer_struct *helloIdleTmr;
static evm_ids_struct helloIdleTmr_evm_ids = {
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

static evm_timer_struct * hello_startIdle_timer(evm_timer_struct *tmr, time_t tv_sec, long tv_nsec, void *ctx_ptr)
{
	evm_log_info("(entry) tmr=%p, sec=%ld, nsec=%ld, ctx_ptr=%p\n", tmr, tv_sec, tv_nsec, ctx_ptr);
	evm_timer_stop(tmr);
	return evm_timer_start(&evs_init, helloIdleTmr_evm_ids, tv_sec, tv_nsec, ctx_ptr);
}

/* HELLO event handlers */
static int evHelloMsg(void *ev_ptr)
{
	evm_message_struct *msg = (evm_message_struct *)ev_ptr;

	evm_log_info("(cb entry) ev_ptr=%p\n", ev_ptr);
	evm_log_notice("HELLO msg received: \"%s\"\n", (char *)msg->iov_buff.iov_base);

	helloIdleTmr = hello_startIdle_timer(helloIdleTmr, 10, 0, NULL);
	evm_log_notice("IDLE timer set: 10 s\n");

	return 0;
}

static int evHelloTmrIdle(void *ev_ptr)
{
	static unsigned int count;
	int status = 0;

	evm_log_info("(cb entry) ev_ptr=%p\n", ev_ptr);
	evm_log_notice("IDLE timer expired!\n");

	hello2_send_hello(sock);

	return status;
}

static int hello2_fork_and_connect(void)
{
	int sd[2];
	pid_t child_pid;

	evm_log_info("(entry)\n");
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sd) == -1)
		evm_log_return_system_err("socketpair()\n");

	child_pid = fork();
	switch (child_pid) {
	case -1: /*error*/
		evm_log_return_system_err("fork()\n");
	case 0: /*child*/
		child = 1;
		close(sd[0]);
		return sd[1];
	}

	/*parent*/
	close(sd[1]);
	return sd[0];
}

static int hello2_send_hello(int sock)
{
	struct msghdr msg;
	int err_save = EINVAL;
	int ret;

	evm_log_info("(entry) sockfd=%d\n", sock);

	/* Prepare message buffer. */
	sprintf((char *)helloMsg.iov_buff.iov_base, "%s: %d", hello_str, ++hello_count);
	helloMsg.iov_buff.iov_len = strlen(send_buff);
	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &helloMsg.iov_buff;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	/* Send HELLO message. */
	ret = sendmsg(sock, &msg, 0);
	if (ret == -1) {
		err_save = errno;
		if (err_save != EINTR) {
			evm_log_system_error("sendmsg(): sockfd=%d\n", sock);
		}
		return -err_save;
	}
	evm_log_notice("HELLO msg sent: \"%s\"\n", (char *)helloMsg.iov_buff.iov_base);

	evm_log_debug("sent=%d\n", ret);
	return ret;
}

static int hello2_receive(int sock, evm_message_struct *message)
{
	struct msghdr msg;
	char *recv_ptr;
	int err_save = EINVAL;
	int ret;

	evm_log_info("(cb entry) sockfd=%d, message=%p\n", sock, message);
	if (message == NULL) {
		errno = err_save;
		evm_log_error("NULL pointer provided for message structure\n");
		return -err_save;
	}

	/* Prepare receive buffer. */
	recv_buff[0] = '\0';
	message->iov_buff.iov_base = recv_buff;
	message->iov_buff.iov_len = MAX_BUFF_SIZE;
	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &message->iov_buff;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	/* Receive whatever message comes in. */
	ret = recvmsg(sock, &msg, 0);
	if (ret == -1) {
		err_save = errno;
		if (err_save != EINTR) {
			evm_log_system_error("recvmsg(): sockfd=%d\n", sock);
		}
		return -err_save;
	}
	message->iov_buff.iov_len = ret;
	((char *)message->iov_buff.iov_base)[ret] = '\0';

	evm_log_debug("buffered=%zd, received data: %s\n", message->iov_buff.iov_len, (char *)message->iov_buff.iov_base);
	return ret;
}

/*
 * Over symplified message parsing:)
 */
static int hello2_parse_message(void *ptr)
{
	evm_message_struct *message = (evm_message_struct *)ptr;

	evm_log_info("(cb entry) ptr=%p\n", ptr);
	if (message == NULL)
		return -EINVAL;

	evm_log_debug("message->iov_buff.iov_len=%zd\n", message->iov_buff.iov_len);
	if (message->iov_buff.iov_len == 0) {
		evm_log_debug("No event decoded (empty receive buffer).\n");
		return -1;
	} else {
		/* Decode the INPUT buffer. */ 
		((char *)message->iov_buff.iov_base)[message->iov_buff.iov_len] = '\0';
		if (strncmp((char *)message->iov_buff.iov_base, hello_str, strlen(hello_str)) == 0) {
			message->msg_ids.ev_id = helloMsg.msg_ids.ev_id;
			message->msg_ids.evm_idx = helloMsg.msg_ids.evm_idx;
			sscanf((char *)message->iov_buff.iov_base, "HELLO: %d", &hello_count);
		} else {
			evm_log_debug("No event decoded (unknown data: %s).\n", (char *)message->iov_buff.iov_base);
			return -1;
		}
	}

	evm_log_debug("message->msg_ids.ev_id=%d, message->msg_ids.evm_idx=%d\n", message->msg_ids.ev_id, message->msg_ids.evm_idx);
	return message->msg_ids.ev_id;
}

/* EVM initialization */
static int hello2_evm_init(void)
{
	int status = 0;

	evm_log_info("(entry)\n");

	sock = hello2_fork_and_connect();

	/* Initialize event machine... */
	evs_init.evm_sigpost = &evs_sigpost;;
	evs_init.evm_link = evs_linkage;
	evs_init.evm_link_max = sizeof(evs_linkage) / sizeof(evm_link_struct) - 1;
	evs_init.evm_tab = evm_tbl;
	evs_init.epoll_max_events = MAX_EPOLL_EVENTS_PER_RUN;
	evm_log_debug("evs_linkage index size = %d\n", evs_init.evm_link_max);
	if ((status = evm_init(&evs_init)) < 0) {
		return status;
	}
	evm_log_debug("evm epoll FD is %d\n", evs_init.epollfd);

	/* Prepare socket FD for EVM to operate over internal socket connection. */
	evs_fd.fd = sock;
	evs_fd.ev_type = EV_TYPE_HELLO_MSG;
	evs_fd.ev_epoll.events = EPOLLIN;
	evs_fd.ev_epoll.data.ptr = (void *)&evs_fd /*our own address*/;
	evs_fd.msg_receive = hello2_receive;
//	evs_fd.msg_send = NULL;
	if ((status = evm_message_fd_add(&evs_init, &evs_fd)) < 0) {
		return status;
	}

	evm_log_info("(exit)\n");
	return 0;
}

/* Main core processing (event loop) */
static int hello2_evm_run(void)
{
	evm_log_info("(entry) child=%d\n", child);

	/* Send first HELLO to the child process! */
	if (!child) {
		hello2_send_hello(sock);
	}

	/*
	 * Main EVM processing (event loop)
	 */
	return evm_run(&evs_init);
}

