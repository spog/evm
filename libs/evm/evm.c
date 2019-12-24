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

#ifndef EVM_FILE_evm_c
#define EVM_FILE_evm_c
#else
#error Preprocesor macro EVM_FILE_evm_c conflict!
#endif

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

#include "evm.h"
#include "timers.h"
#include "messages.h"

#include "userlog/log_module.h"
EVMLOG_MODULE_INIT(EVM_CORE, 1)

static int evm_prepare_event(evm_evids_list_struct *evm_tmr, void *ev_ptr);
static int evm_handle_event(evm_evids_list_struct *evm_tmr, void *ev_ptr);
static int evm_finalize_event(evm_evids_list_struct *evm_tmr, void *ev_ptr);

static int evm_handle_timer(evm_timer_struct *expdTmr);
static int evm_handle_message(evm_message_struct *recvdMsg);

/*current project version*/
unsigned int evm_version_major = EVM_VERSION_MAJOR;
unsigned int evm_version_minor = EVM_VERSION_MINOR;
unsigned int evm_version_patch = EVM_VERSION_PATCH;

/*
 * Main event machine initialization
 */
int evm_init(evm_init_struct *evm_init_ptr)
{
	int status = -1;

	evm_log_info("(entry) evm_init_ptr=%p\n", evm_init_ptr);
	/* Initialize timers infrastructure... */
	if ((status = evm_timers_init(evm_init_ptr)) < 0) {
		return status;
	}

	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
		return status;
	}

	/* Initialize EVM messages infrastructure... */
	if ((status = evm_messages_init(evm_init_ptr)) < 0) {
		return status;
	}

	/* Initialize internal timers queue FD... */
	if ((status = evm_timers_queue_fd_init(evm_init_ptr)) < 0) {
		return status;
	}

	/* Initialize internal messages queue FD... */
	if ((status = evm_messages_queue_fd_init(evm_init_ptr)) < 0) {
		return status;
	}

	return 0;
}

/*
 * Main event machine single pass-through (asynchronous, if epoll_pwait timeout = 0)
 */
int evm_run_once(evm_init_struct *evm_init_ptr)
{
	int status = 0;
	evm_timer_struct *expdTmr;
	evm_message_struct *recvdMsg;

	evm_log_info("(entry)\n");
	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
		abort();
	}

	/* Loop exclusively over expired timers (non-blocking already)! */
	for (;;) {
		evm_log_info("(main loop entry)\n");
		/* Handle expired timer (NON-BLOCKING). */
		if ((expdTmr = evm_timers_check(evm_init_ptr)) != NULL) {
			if ((status = evm_handle_timer(expdTmr)) < 0)
				evm_log_debug("evm_handle_timer() returned %d\n", status);
		} else
			break;
	}

	/* Handle handle received message (WAIT - THE ONLY BLOCKING POINT, if epoll_pwait timeout != 0). */
	if ((recvdMsg = evm_messages_check(evm_init_ptr)) != NULL) {
		if ((status = evm_handle_message(recvdMsg)) < 0)
			evm_log_debug("evm_handle_message() returned %d\n", status);
	}

	return 0;
}

/*
 * Main event machine asynchronous call
 */
int evm_run_async(evm_init_struct *evm_init_ptr)
{
	int status = 0;
	evm_timer_struct *expdTmr;
	evm_message_struct *recvdMsg;

	evm_log_info("(entry)\n");
	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
		abort();
	}

	evm_init_ptr->epoll_timeout = 0;

	return evm_run_once(evm_init_ptr);
}

/*
 * Main event machine loop
 */
int evm_run(evm_init_struct *evm_init_ptr)
{
	int status = 0;
	evm_timer_struct *expdTmr;
	evm_message_struct *recvdMsg;

	evm_log_info("(entry)\n");
	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
		abort();
	}

	/* Main protocol loop! */
	for (;;) {
		evm_log_info("(main loop entry)\n");
		/* Handle expired timer (NON-BLOCKING). */
		if ((expdTmr = evm_timers_check(evm_init_ptr)) != NULL) {
			if ((status = evm_handle_timer(expdTmr)) < 0)
				evm_log_debug("evm_handle_timer() returned %d\n", status);
			continue;
		}

		/* Handle handle received message (WAIT - THE ONLY BLOCKING POINT). */
		if ((recvdMsg = evm_messages_check(evm_init_ptr)) != NULL) {
			if ((status = evm_handle_message(recvdMsg)) < 0)
				evm_log_debug("evm_handle_message() returned %d\n", status);
		}
	}

	return 0;
}

static int evm_prepare_event(evm_evids_list_struct *evm_ev, void *ev_ptr)
{
	if (evm_ev != NULL) {
		if (evm_ev->ev_prepare != NULL)
			return evm_ev->ev_prepare(ev_ptr);
		else
			return 0;
	}
	return -1;
}

static int evm_handle_event(evm_evids_list_struct *evm_ev, void *ev_ptr)
{
	if (evm_ev != NULL)
		if (evm_ev->ev_handle != NULL)
			return evm_ev->ev_handle(ev_ptr);
	return -1;
}

static int evm_finalize_event(evm_evids_list_struct *evm_ev, void *ev_ptr)
{
	if (evm_ev != NULL)
		if (evm_ev->ev_finalize != NULL)
			return evm_ev->ev_finalize(ev_ptr);
	return -1;
}

static int evm_handle_timer(evm_timer_struct *expdTmr)
{
	int status1 = 0;
	int status2 = 0;

	if (expdTmr == NULL)
		return -1;

	if (expdTmr->stopped == 0) {
		status1 = evm_handle_event(expdTmr->tmrid_ptr, expdTmr);
		if (status1 < 0)
			evm_log_debug("evm_handle_event() returned %d\n", status1);
	}

	status2 = evm_finalize_event(expdTmr->tmrid_ptr, expdTmr);
	if (status2 < 0)
		evm_log_debug("evm_handle_event() returned %d\n", status2);

	return (status1 | status2);
}

static int evm_handle_message(evm_message_struct *recvdMsg)
{
	int status0 = 0;
	int status1 = 0;
	int status2 = 0;

	status0 = evm_prepare_event(recvdMsg->msgid_ptr, recvdMsg);
	if (status0 < 0)
		evm_log_debug("evm_prepare_event() returned %d\n", status0);

	status1 = evm_handle_event(recvdMsg->msgid_ptr, recvdMsg);
	if (status1 < 0)
		evm_log_debug("evm_handle_event() returned %d\n", status1);

	status2 = evm_finalize_event(recvdMsg->msgid_ptr, recvdMsg);
	if (status2 < 0)
		evm_log_debug("evm_finalize_event() returned %d\n", status2);

	return (status0 | status1 | status2);
}

