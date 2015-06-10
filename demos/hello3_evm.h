/*
 * The hello3_evm demo program
 *
 * Copyright (C) 2014 Samo Pogacnik <samo_pogacnik@t-2.net>
 * All rights reserved.
 *
 * This file is part of the EVM software project.
 * This file is provided under the terms of the BSD 3-Clause license,
 * available in the LICENSE file of the EVM software project.
*/

#ifndef hello3_evm_h
#define hello3_evm_h

#ifdef hello3_evm_c
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

#ifdef hello3_evm_c
/*
 * Here is the PRIVATE stuff (within above ifdef).
 * It is here so we make sure, that the following PRIVATE stuff get included into own source ONLY!
 */
#include <userlog/log_module.h>
EVMLOG_MODULE_INIT(DEMO3EVM, 2);

#define MAX_EPOLL_EVENTS_PER_RUN 10

static int signal_processing(int sig, void *ptr);

enum event_msg_types {
	EV_TYPE_UNKNOWN_MSG = 0,
	EV_TYPE_HELLO_MSG
};
enum event_tmr_types {
	EV_TYPE_UNKNOWN_TMR = 0,
	EV_TYPE_HELLO_TMR
};

enum hello_msg_ev_ids {
	EV_ID_HELLO_MSG_HELLO = 0
};
enum hello_tmr_ev_ids {
	EV_ID_HELLO_TMR_IDLE = 0,
	EV_ID_HELLO_TMR_QUIT
};

static evm_timer_struct * hello_start_timer(int evm_id, evm_timer_struct *tmr, time_t tv_sec, long tv_nsec, void *ctx_ptr, int tmr_type, int tmr_id);
static int hello3_send_hello(evm_init_struct *evm_ptr);

static int evHelloMsg(void *ev_ptr);
static int evHelloTmrIdle(void *ev_ptr);
static int evHelloTmrQuit(void *ev_ptr);

static int hello3_evm_init(void);
static int hello3_evm_run(void);

#endif /*hello3_evm_c*/
/*
 * Here continues the PUBLIC stuff, if necessary.
 */

#define MAX_BUFF_SIZE 1024

#endif /*hello3_evm_h*/
