/*
 * The EVM timers module
 *
 * Copyright (C) 2014 Samo Pogacnik <samo_pogacnik@t-2.net>
 * All rights reserved.
 *
 * This file is part of the EVM software project.
 * This file is provided under the terms of the BSD 3-Clause license,
 * available in the LICENSE file of the EVM software project.
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

#include "evm/libevm.h"

EXTERN int evm_timers_init(evm_init_struct *evm_init_ptr);
EXTERN evm_timer_struct * evm_timers_check(evm_init_struct *evm_init_ptr);

#ifdef evm_timers_c
/*
 * Here is the PRIVATE stuff (within above ifdef).
 * It is here so we make sure, that the following PRIVATE stuff get included into own source ONLY!
 */
#include "evm/log_module.h"
EVMLOG_MODULE_INIT(EVM_TMRS, 1)


#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

typedef struct timer_queue timer_queue_struct;

struct timer_queue {
	evm_timer_struct *first_tmr;
};

#endif /*evm_timers_c*/
/*
 * Here continues the PUBLIC stuff, if necessary.
 */

#endif /*evm_timers_h*/
