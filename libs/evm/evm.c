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

#ifndef evm_c
#define evm_c
#else
#error Preprocesor macro evm_c conflict!
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

#include "userlog/log_module.h"
EVMLOG_MODULE_INIT(EVM_CORE, 1)

#include "evm.h"
#include "timers.h"
#include "messages.h"

static int evm_prepare_event(evm_tab_struct *evm_tab, int idx, void *ev_ptr);
static int evm_handle_event(evm_tab_struct *evm_tab, int idx, void *ev_ptr);
static int evm_finalize_event(evm_tab_struct *evm_tab, int idx, void *ev_ptr);

static int evm_handle_timer(evm_timer_struct *expdTmr);
static int evm_handle_message(evm_message_struct *recvdMsg);

/*current project version*/
unsigned int evm_version_major = EVM_VERSION_MAJOR;
unsigned int evm_version_minor = EVM_VERSION_MINOR;
unsigned int evm_version_patch = EVM_VERSION_PATCH;

/*
 * Event machine linkage initialization
 */
static int evm_link_init(evm_init_struct *evm_init_ptr)
{
	int i, num_evs, status = -1;
	int ev_msgs_type_max = 0;
	int ev_tmrs_type_max = 0;
	evm_msgs_link_struct *evm_msgs_linkage;
	evm_tmrs_link_struct *evm_tmrs_linkage;
	evm_tab_struct *evm_table;

	evm_log_info("(entry) evm_init_ptr=%p\n", evm_init_ptr);
	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
		return status;
	}

	evm_log_info("evm_init_ptr->evm_relink=%d\n", evm_init_ptr->evm_relink);
	evm_msgs_linkage = evm_init_ptr->evm_msgs_link;
	ev_msgs_type_max = evm_init_ptr->evm_msgs_link_max;
	if (ev_msgs_type_max < 0) {
		evm_log_error("Wrong evm_msgs_linkage table max index = %d\n", ev_msgs_type_max);
		return status;
	}
	if (evm_msgs_linkage == NULL) {
		evm_log_error("Message types linkage table empty - event machine init failed!\n");
		return status;
	}

	evm_tmrs_linkage = evm_init_ptr->evm_tmrs_link;
	ev_tmrs_type_max = evm_init_ptr->evm_tmrs_link_max;
	if (ev_tmrs_type_max < 0) {
		evm_log_error("Wrong evm_tmrs_linkage table max index = %d\n", ev_tmrs_type_max);
		return status;
	}
	if (evm_tmrs_linkage == NULL) {
		evm_log_error("Timer types linkage table empty - event machine init failed!\n");
		return status;
	}

	for (i = 0; i <= ev_msgs_type_max; i++) {
		if (evm_msgs_linkage[i].msgs_tab  == NULL) {
			num_evs = (evm_msgs_linkage[i].last_ev_id - evm_msgs_linkage[i].first_ev_id + 1);
			if ((evm_msgs_linkage[i].msgs_tab = (evm_message_struct *)calloc(num_evs, sizeof(evm_message_struct))) == NULL) {
				errno = ENOMEM;
				evm_log_system_error("calloc(): msgs_tab\n");
				return status;
			}
			evm_msgs_linkage[i].linked_msgs = 0;
		} else {
			evm_log_info("Already allocated msgs_tab for message type (linked_msgs = %d)!\n", evm_msgs_linkage[i].linked_msgs);
		}
	}

	for (i = 0; i <= ev_tmrs_type_max; i++) {
		if (evm_tmrs_linkage[i].tmrs_tab  == NULL) {
			num_evs = (evm_tmrs_linkage[i].last_ev_id - evm_tmrs_linkage[i].first_ev_id + 1);
			if ((evm_tmrs_linkage[i].tmrs_tab = (evm_timer_struct *)calloc(num_evs, sizeof(evm_timer_struct))) == NULL) {
				errno = ENOMEM;
				evm_log_system_error("calloc(): tmrs_tab\n");
				return status;
			}
			evm_tmrs_linkage[i].linked_tmrs = 0;
		} else {
			evm_log_info("Already allocated tmrs_tab for timer type (linked_tmrs = %d)!\n", evm_tmrs_linkage[i].linked_tmrs);
		}
	}

	if (evm_init_ptr->evm_relink != 0) {
		evm_table = evm_init_ptr->evm_msgs_tab;
		if (evm_table == NULL) {
			evm_log_error("Messages event table empty - event machine init failed!\n");
			return status;
		}

		i = 0;
		while (evm_table[i].ev_handle != NULL) {
			evm_log_debug("(relink evm_msgs_tab) i=%d\n", i);
			if (evm_table[i].ev_type <= ev_msgs_type_max) {
				evm_msgs_linkage[evm_table[i].ev_type].msgs_tab[evm_table[i].ev_id - evm_msgs_linkage[evm_table[i].ev_type].first_ev_id].msg_ids.ev_id = evm_table[i].ev_id;
				evm_msgs_linkage[evm_table[i].ev_type].msgs_tab[evm_table[i].ev_id - evm_msgs_linkage[evm_table[i].ev_type].first_ev_id].msg_ids.evm_idx = i;
				evm_msgs_linkage[evm_table[i].ev_type].linked_msgs++;
			} else {
				errno = EINVAL;
				evm_log_return_err("Messages event table index %d (wrong event type) - event machine init failed!\n", i);
			};
			i++;
		}

		for (i = 0; i <= ev_msgs_type_max; i++) {
			evm_log_info("Allocated and relinked msgs_tab for message type (linked_msgs = %d)!\n", evm_msgs_linkage[i].linked_msgs);
		}

		evm_table = evm_init_ptr->evm_tmrs_tab;
		if (evm_table == NULL) {
			evm_log_error("Timers event table empty - event machine init failed!\n");
			return status;
		}

		i = 0;
		while (evm_table[i].ev_handle != NULL) {
			evm_log_debug("(relink evm_tmrs_tab) i=%d\n", i);
			if (evm_table[i].ev_type <= ev_tmrs_type_max) {
				evm_tmrs_linkage[evm_table[i].ev_type].tmrs_tab[evm_table[i].ev_id - evm_tmrs_linkage[evm_table[i].ev_type].first_ev_id].tmr_ids.ev_id = evm_table[i].ev_id;
				evm_tmrs_linkage[evm_table[i].ev_type].tmrs_tab[evm_table[i].ev_id - evm_tmrs_linkage[evm_table[i].ev_type].first_ev_id].tmr_ids.evm_idx = i;
				evm_tmrs_linkage[evm_table[i].ev_type].linked_tmrs++;
			} else {
				errno = EINVAL;
				evm_log_return_err("Timers event table index %d (wrong event type) - event machine init failed!\n", i);
			};
			i++;
		}

		for (i = 0; i <= ev_tmrs_type_max; i++) {
			evm_log_info("Allocated and relinked tmrs_tab for timer type (linked_tmrs = %d)!\n", evm_tmrs_linkage[i].linked_tmrs);
		}
	}

	return 0;
}

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

	if (evm_init_ptr->evm_msgs_link == NULL) {
		evm_log_error("Message event types linkage table empty - event machine init failed!\n");
		return status;
	}

	if (evm_init_ptr->evm_tmrs_link == NULL) {
		evm_log_error("Timer event types linkage table empty - event machine init failed!\n");
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

	return evm_link_init(evm_init_ptr);
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

	if (evm_init_ptr->evm_msgs_tab == NULL) {
		evm_log_error("Messages event table NOT specified - Starting event machine failed!\n");
		abort();
	}

	if (evm_init_ptr->evm_tmrs_tab == NULL) {
		evm_log_error("Timers event table NOT specified - Starting event machine failed!\n");
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

static int evm_prepare_event(evm_tab_struct *evm_tab, int idx, void *ev_ptr)
{
	if (evm_tab != NULL) {
		if (evm_tab[idx].ev_prepare != NULL)
			return evm_tab[idx].ev_prepare(ev_ptr);
		else
			return 0;
	}
	return -1;
}

static int evm_handle_event(evm_tab_struct *evm_tab, int idx, void *ev_ptr)
{
	if (evm_tab != NULL)
		if (evm_tab[idx].ev_handle != NULL)
			return evm_tab[idx].ev_handle(ev_ptr);
	return -1;
}

static int evm_finalize_event(evm_tab_struct *evm_tab, int idx, void *ev_ptr)
{
	if (evm_tab != NULL)
		if (evm_tab[idx].ev_finalize != NULL)
			return evm_tab[idx].ev_finalize(ev_ptr);
	return -1;
}

static int evm_handle_timer(evm_timer_struct *expdTmr)
{
	int status1 = 0;
	int status2 = 0;

	if (expdTmr == NULL)
		return -1;

	if (expdTmr->stopped == 0) {
		status1 = evm_handle_event(expdTmr->evm_ptr->evm_tmrs_tab, expdTmr->tmr_ids.evm_idx, expdTmr);
		if (status1 < 0)
			evm_log_debug("evm_handle_event() returned %d\n", status1);
	}

	status2 = evm_finalize_event(expdTmr->evm_ptr->evm_tmrs_tab, expdTmr->tmr_ids.evm_idx, expdTmr);
	if (status2 < 0)
		evm_log_debug("evm_handle_event() returned %d\n", status2);

	return (status1 | status2);
}

static int evm_handle_message(evm_message_struct *recvdMsg)
{
	int status0 = 0;
	int status1 = 0;
	int status2 = 0;

	status0 = evm_prepare_event(recvdMsg->evm_ptr->evm_msgs_tab, recvdMsg->msg_ids.evm_idx, recvdMsg);
	if (status0 < 0)
		evm_log_debug("evm_prepare_event() returned %d\n", status0);

	status1 = evm_handle_event(recvdMsg->evm_ptr->evm_msgs_tab, recvdMsg->msg_ids.evm_idx, recvdMsg);
	if (status1 < 0)
		evm_log_debug("evm_handle_event() returned %d\n", status1);

	status2 = evm_finalize_event(recvdMsg->evm_ptr->evm_msgs_tab, recvdMsg->msg_ids.evm_idx, recvdMsg);
	if (status2 < 0)
		evm_log_debug("evm_finalize_event() returned %d\n", status2);

	return (status0 | status1 | status2);
}

