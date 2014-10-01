/*
 *  Copyright (C) 2012  Samo Pogacnik
 */

/*
 * The EVM timers module
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

EXTERN int evm_timers_init(void);
EXTERN evm_timer_struct * evm_timers_check(void);

#ifdef evm_timers_c
/*
 * Here is the PRIVATE stuff (within above ifdef).
 * It is here so we make sure, that the following PRIVATE stuff get included into own source ONLY!
 */
#include "evm/log_module.h"
EVMLOG_MODULE_INIT(EVM_TMRS, 1)


#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

#endif /*evm_timers_c*/
/*
 * Here continues the PUBLIC stuff, if necessary.
 */

#endif /*evm_timers_h*/
