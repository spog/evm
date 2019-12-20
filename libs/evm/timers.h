/*
 * The EVM timers module
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

#ifndef evm_timers_h
#define evm_timers_h

#ifdef evm_timers_c
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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>

#include "evm/libevm.h"

EXTERN int evm_timers_init(evm_init_struct *evm_init_ptr);
EXTERN int evm_timers_queue_fd_init(evm_init_struct *evm_init_ptr);
EXTERN evm_timer_struct * evm_timers_check(evm_init_struct *evm_init_ptr);

#ifdef evm_timers_c
/*
 * Here is the PRIVATE stuff (within above ifdef).
 * It is here so we make sure, that the following PRIVATE stuff get included into own source ONLY!
 */
#include "userlog/log_module.h"
EVMLOG_MODULE_INIT(EVM_TMRS, 1)


#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

typedef struct timer_queue timer_queue_struct;

struct timer_queue {
	evm_timer_struct *first_tmr;
	evm_timer_struct *last_tmr;
	evm_fd_struct *evmfd; /*internal timer queue FD binding - eventfd()*/
	pthread_mutex_t mutex;
};

static int timer_queue_evmfd_read(int efd, evm_message_struct *ptr);

#endif /*evm_timers_c*/
/*
 * Here continues the PUBLIC stuff, if necessary.
 */

#endif /*evm_timers_h*/
