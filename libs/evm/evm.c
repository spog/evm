/*
 *  Copyright (C) 2012  Samo Pogacnik
 */

/*
 * The event machine (evm) module
 */
#ifndef evm_c
#define evm_c
#else
#error Preprocesor macro evm_c conflict!
#endif

#include "evm.h"

/*
 * Event machine linkage initialization
 */
int evm_link_init(evm_init_struct *evm_init_ptr)
{
	int i = 0, status = -1;
	int ev_type_max = 0;
	evm_link_struct *evm_linkage;
	evm_tab_struct *evm_table;

	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
		return status;
	}

	evm_linkage = evm_init_ptr->evm_link;
	ev_type_max = evm_init_ptr->evm_link_max;
	if (ev_type_max < 0) {
		evm_log_error("Wrong evm_linkage table max index = %d\n", ev_type_max);
		return status;
	}
	if (evm_linkage == NULL) {
		evm_log_error("Event types linkage table empty - event machine init failed!\n");
		return status;
	}

	evm_table = evm_init_ptr->evm_tab;
	if (evm_table == NULL) {
		evm_log_error("Events table empty - event machine init failed!\n");
		return status;
	}

	while (evm_table[i].ev_handle != NULL) {
		if (evm_table[i].ev_type <= ev_type_max) {
			if (evm_linkage[evm_table[i].ev_type].ev_type_link != NULL) {
				if ((status = evm_linkage[evm_table[i].ev_type].ev_type_link(evm_table[i].ev_id, i)) < 0) {
					evm_log_error("Events table index %d (linkage error %d) - event machine init failed!\n", i, status);
					return status;
				}
			} else {
				errno = EINVAL;
				evm_log_return_err("Events table index %d (linkage undefined) - event machine init failed!\n", i);
			};
		} else {
			errno = EINVAL;
			evm_log_return_err("Events table index %d (wrong event type) - event machine init failed!\n", i);
		};
		i++;
	}

	return i;
}

/*
 * Main event machine initialization
 */
int evm_init(evm_init_struct *evm_init_ptr)
{
	int status = -1;
	evm_link_struct *evm_linkage;

	/* Initialize timers infrastructure... */
	if ((status = evm_timers_init()) < 0) {
		return status;
	}

	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
		return status;
	}

	evm_linkage = evm_init_ptr->evm_link;
	if (evm_linkage == NULL) {
		evm_log_error("Event types linkage table empty - event machine init failed!\n");
		return status;
	}

	/* Initialize EVM messages infrastructure... */
	if ((status = evm_messages_init(evm_init_ptr)) < 0) {
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

	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
		abort();
	}

	if (evm_init_ptr->evm_tab == NULL) {
		evm_log_error("Events table NOT specified - Starting event machine failed!\n");
		abort();
	}

	/* Main protocol loop! */
	for (;;) {
		/* Handle expired timer (NON-BLOCKING). */
		if ((expdTmr = evm_timers_check()) != NULL) {
			if ((status = evm_handle_timer(expdTmr)) < 0)
				evm_log_debug("evm_handle_timer() returned %d\n", status);
			continue;
		}

		/* Handle handle received message (WAIT - THE ONLY BLOCKING POINT). */
		if ((recvdMsg = evm_messages_check(evm_init_ptr)) != NULL) {
			recvdMsg->evm_tab = evm_init_ptr->evm_tab;
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
		status1 = evm_handle_event(expdTmr->evm_tab, expdTmr->tmr_ids.evm_idx, expdTmr);
		if (status1 < 0)
			evm_log_debug("evm_handle_event() returned %d\n", status1);
	}

	status2 = evm_finalize_event(expdTmr->evm_tab, expdTmr->tmr_ids.evm_idx, expdTmr);
	if (status2 < 0)
		evm_log_debug("evm_handle_event() returned %d\n", status2);

	return (status1 | status2);
}

static int evm_handle_message(evm_message_struct *recvdMsg)
{
	int status0 = 0;
	int status1 = 0;
	int status2 = 0;

	status0 = evm_prepare_event(recvdMsg->evm_tab, recvdMsg->msg_ids.evm_idx, recvdMsg);
	if (status0 < 0)
		evm_log_debug("evm_prepare_event() returned %d\n", status0);

	status1 = evm_handle_event(recvdMsg->evm_tab, recvdMsg->msg_ids.evm_idx, recvdMsg);
	if (status1 < 0)
		evm_log_debug("evm_handle_event() returned %d\n", status1);

	status2 = evm_finalize_event(recvdMsg->evm_tab, recvdMsg->msg_ids.evm_idx, recvdMsg);
	if (status2 < 0)
		evm_log_debug("evm_finalize_event() returned %d\n", status2);

	return (status0 | status1 | status2);
}

void evm_message_pass(evm_message_struct *msg)
{
	evm_message_enqueue(msg);
}

int evm_message_fd_add(evm_init_struct *evm_init_ptr, evm_fd_struct *evm_fd_ptr)
{
	if (evm_init_ptr == NULL)
		return -1;

	if (evm_fd_ptr == NULL)
		return -1;

	evm_fd_ptr->msg_ptr = (evm_message_struct *)calloc(1, sizeof(evm_message_struct));
	if (evm_fd_ptr->msg_ptr == NULL) {
		errno = ENOMEM;
		evm_log_return_err("calloc(): 1 times %zd bytes\n", sizeof(evm_message_struct));
	}
	evm_log_debug("evm_fd_ptr: %p, &evm_fd_ptr->ev_epoll: %p\n", evm_fd_ptr, &evm_fd_ptr->ev_epoll);
	evm_log_debug("evm_init_ptr->evm_epollfd: %d, evm_fd_ptr->fd: %d\n", evm_init_ptr->evm_epollfd, evm_fd_ptr->fd);
	if (epoll_ctl(evm_init_ptr->evm_epollfd, EPOLL_CTL_ADD, evm_fd_ptr->fd, &evm_fd_ptr->ev_epoll) < 0) {
		evm_log_return_system_err("epoll_ctl()\n");
	}
}

