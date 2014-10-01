/*
 *  Copyright (C) 2012  Samo Pogacnik
 */

/*
 * The event machine (evm) module.
 */

#ifndef evm_h
#define evm_h

#ifdef evm_c
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
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "evm/libevm.h"

#ifdef evm_c
/*
 * Here is the PRIVATE stuff (within above ifdef).
 * It is here so we make sure, that the following PRIVATE stuff get included into own source ONLY!
 */
#include "evm/log_module.h"
EVMLOG_MODULE_INIT(EVM_CORE, 1)

#include "timers.h"
#include "messages.h"

static int evm_prepare_event(evm_tab_struct *evm_tab, int idx, void *ev_ptr);
static int evm_handle_event(evm_tab_struct *evm_tab, int idx, void *ev_ptr);
static int evm_finalize_event(evm_tab_struct *evm_tab, int idx, void *ev_ptr);

static int evm_handle_timer(evm_timer_struct *expdTmr);
static int evm_handle_message(evm_message_struct *recvdMsg);

#endif /*evm_c*/
/*
 * Here continues the PUBLIC stuff, if necessary.
 */

#endif /*evm_h*/
