/*
 *  Copyright (C) 2012  Samo Pogacnik
 */

/*
 * The timers module
 */

#ifndef timers_h
#define timers_h

#ifdef timers_c
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

#define COMP_MODULE libevm_module
EXTERN unsigned int COMP_MODULE;
#include "evm/log_conf.h"

#include "evm/libevm.h"

EXTERN int timers_init(void);
EXTERN struct timer_struct * timers_check(void);

#ifdef timers_c
/*
 * Here is the PRIVATE stuff (within above ifdef).
 * It is here so we make sure, that the following PRIVATE stuff get included into own source ONLY!
 */

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

#endif /*timers_c*/
/*
 * Here continues the PUBLIC stuff, if necessary.
 */

#endif /*timers_h*/
