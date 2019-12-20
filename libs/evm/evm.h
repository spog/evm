/*
 * The event machine (evm) module
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
#include "userlog/log_module.h"
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
