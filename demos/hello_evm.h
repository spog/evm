/*
 *  Copyright (C) 2012  Samo Pogacnik
 */

/*
 * The hello_evm demo
 */

#ifndef hello_evm_h
#define hello_evm_h

#ifdef hello_evm_c
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

#ifdef hello_evm_c
/*
 * Here is the PRIVATE stuff (within above ifdef).
 * It is here so we make sure, that the following PRIVATE stuff get included into own source ONLY!
 */
#include <evm/log_module.h>
EVMLOG_MODULE_INIT(DEMO_EVM, 2);

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
};

static int hello_messages_link(int ev_id, int evm_idx);
static int helloTmrs_link(int ev_id, int evm_idx);

static int evHelloMsg(void *ev_ptr);
static int evHelloTmrIdle(void *ev_ptr);

static int hello_evm_init(void);
static int hello_evm_run(void);

#endif /*hello_evm_c*/
/*
 * Here continues the PUBLIC stuff, if necessary.
 */

#define MAX_BUFF_SIZE 1024

#endif /*hello_evm_h*/
