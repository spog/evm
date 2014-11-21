/*
 * The EVM messages module
 *
 * Copyright (C) 2014 Samo Pogacnik <samo_pogacnik@t-2.net>
 * All rights reserved.
 *
 * This file is part of the EVM software project.
 * This file is provided under the terms of the BSD 3-Clause license,
 * available in the LICENSE file of the EVM software project.
*/

#ifndef evm_messages_h
#define evm_messages_h

#ifdef evm_messages_c
/* PRIVATE usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN
#else
/* PUBLIC usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN extern
#endif
/*
 * Here starts the PUBLIC stuff:
 *	Enter PUBLIC declarations using EXTERN!
 */
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>

#include "evm/libevm.h"

EXTERN int evm_messages_init(evm_init_struct *evm_init_ptr);
EXTERN evm_message_struct * evm_messages_check(evm_init_struct *evm_init_ptr);

EXTERN void evm_message_enqueue(evm_init_struct *evm_init_ptr, evm_message_struct *msg);

#ifdef evm_messages_c
/*
 * Here is the PRIVATE stuff (within above ifdef).
 * It is here so we make sure, that the following PRIVATE stuff get included into own source ONLY!
 */
#include "evm/log_module.h"
EVMLOG_MODULE_INIT(EVM_MSGS, 1)

typedef struct message_queue message_queue_struct;

struct message_queue {
	evm_message_struct *first_msg;
	evm_message_struct *last_msg;
};

static void messages_sighandler(int signum, siginfo_t *siginfo, void *context);
static int messages_sighandler_install(int signum);
static evm_fd_struct * messages_epoll(evm_init_struct *evm_init_ptr);
static int messages_receive(evm_fd_struct *evs_fd_ptr, evm_link_struct *evm_linkage);
static int messages_parse(evm_fd_struct *evs_fd_ptr, evm_link_struct *evm_linkage);
static evm_message_struct * message_dequeue(evm_init_struct *evm_init_ptr);

#endif /*evm_messages_c*/
/*
 * Here continues the PUBLIC stuff, if necessary.
 */

#endif /*evm_messages_h*/
