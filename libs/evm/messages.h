/*
 * The EVM messages module
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
EXTERN int evm_messages_queue_fd_init(evm_init_struct *evm_init_ptr);
EXTERN evm_message_struct * evm_messages_check(evm_init_struct *evm_init_ptr);

EXTERN int evm_message_enqueue(evm_init_struct *evm_init_ptr, evm_message_struct *msg);

#ifdef evm_messages_c
/*
 * Here is the PRIVATE stuff (within above ifdef).
 * It is here so we make sure, that the following PRIVATE stuff get included into own source ONLY!
 */
#include "userlog/log_module.h"
EVMLOG_MODULE_INIT(EVM_MSGS, 1)

typedef struct message_queue message_queue_struct;
typedef struct message_hanger message_hanger_struct;

struct message_queue {
	message_hanger_struct *first_hanger;
	message_hanger_struct *last_hanger;
	evm_fd_struct *evmfd; /*internal message queue FD binding - eventfd()*/
	pthread_mutex_t mutex;
};

struct message_hanger {
	message_hanger_struct *next;
	message_hanger_struct *prev;
	evm_message_struct *msg; /*hangs of a hanger when linked in a chain - i.e.: in a message queue*/
};

static void messages_sighandler(int signum, siginfo_t *siginfo, void *context);
static int messages_sighandler_install(int signum);
static evm_fd_struct * messages_epoll(evm_init_struct *evm_init_ptr);
static int messages_receive(evm_fd_struct *evs_fd_ptr, evm_msgs_link_struct *evm_linkage);
static int messages_parse(evm_fd_struct *evs_fd_ptr, evm_msgs_link_struct *evm_linkage);
static evm_message_struct * message_dequeue(evm_init_struct *evm_init_ptr);
static int message_queue_evmfd_read(int efd, evm_message_struct *message);

#endif /*evm_messages_c*/
/*
 * Here continues the PUBLIC stuff, if necessary.
 */

#endif /*evm_messages_h*/
