/*
 * The hello4_evm demo program
 *
 * Copyright (C) 2014 Samo Pogacnik <samo_pogacnik@t-2.net>
 * All rights reserved.
 *
 * This file is part of the EVM software project.
 * This file is provided under the terms of the BSD 3-Clause license,
 * available in the LICENSE file of the EVM software project.
*/

#ifndef hello4_evm_h
#define hello4_evm_h

#ifdef hello4_evm_c
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

#ifdef hello4_evm_c
/*
 * Here is the PRIVATE stuff (within above ifdef).
 * It is here so we make sure, that the following PRIVATE stuff get included into own source ONLY!
 */
#include <evm/log_module.h>
EVMLOG_MODULE_INIT(DEMO3EVM, 2);

#define MAX_EPOLL_EVENTS_PER_RUN 10

static int signal_processing(int sig, void *ptr);

#define EV_ID_FIRST 0
enum hello_msg_ev_ids {
	EV_ID_HELLO_MSG_HELLO = EV_ID_FIRST
};
enum event_types {
	EV_TYPE_UNKNOWN = 0,
	EV_TYPE_HELLO_MSG,
	EV_TYPE_HELLO_TMR
};
#define EV_TYPE_MAX EV_TYPE_HELLO_TMR

enum hello_tmr_ev_ids {
	EV_ID_HELLO_TMR_IDLE = 0,
	EV_ID_HELLO_TMR_QUIT
};

static int hello4_send_hello(evm_init_struct *loc_evm_ptr, evm_init_struct *rem_evm_ptr);
static int hello_messages_link(int ev_id, int evm_idx);
static int helloTmrs_link(int ev_id, int evm_idx);

static int evHelloMsg(void *ev_ptr);
static int evHelloTmrIdle(void *ev_ptr);
static int evHelloTmrQuit(void *ev_ptr);

static evm_init_struct * hello4_evm_init(void);
static int hello4_evm_run(evm_init_struct *ptr);

#endif /*hello4_evm_c*/
/*
 * Here continues the PUBLIC stuff, if necessary.
 */

#define MAX_BUFF_SIZE 1024

#endif /*hello4_evm_h*/
