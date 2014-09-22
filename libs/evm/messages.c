/*
 *  Copyright (C) 2012  Samo Pogacnik
 */

/*
 * The messages module
 */
#ifndef messages_c
#define messages_c
#else
#error Preprocesor macro messages_c conflict!
#endif

#include "messages.h"

static sem_t semaphore;
static unsigned int evm_signum = 0;
static sigset_t evm_sigmask;

static void sighandler(int signum, siginfo_t *siginfo, void *context)
{
	sigfillset(&evm_sigmask);
	if (sigprocmask(SIG_BLOCK, &evm_sigmask, NULL) < 0) {
		evm_signum = 0;
	} else {
		evm_signum = signum;
	}
	sem_post(&semaphore);
}

static struct message_queue_struct evm_msg_queue = {
	.first_msg = NULL,
	.last_msg = NULL,
};

int messages_init(struct evm_init_struct *evm_init_ptr)
{
	int status = -1;
	struct sigaction act;
	struct evm_link_struct *evm_linkage;
	struct evm_fds_struct *evm_fds;
	struct evm_sigpost_struct *evm_sigpost;

	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
		return status;
	}

	evm_linkage = evm_init_ptr->evm_link;
	if (evm_linkage == NULL) {
		evm_log_error("Event types linkage table empty - event machine init failed!\n");
		return status;
	}

	evm_fds = evm_init_ptr->evm_fds;
	if (evm_fds == NULL) {
		evm_log_error("FDs table empty - event machine init failed!\n");
		return status;
	}

	evm_sigpost = evm_init_ptr->evm_sigpost;
	if (evm_sigpost == NULL)
		evm_log_debug("Signal post-processing handler undefined!\n");

	if (sem_init(&semaphore, 0, 0) == -1)
		evm_log_return_system_err("sem_init()\n");

	sigfillset(&evm_sigmask);
	if (sigprocmask(SIG_UNBLOCK, &evm_sigmask, NULL) < 0) {
		evm_log_return_system_err("sigprocmask() SIG_UNBLOCK\n");
	}

	memset (&act, '\0', sizeof(act));
	/* Use the sa_sigaction field because the handles has two additional parameters */
	act.sa_sigaction = &sighandler;
	/* The SA_SIGINFO flag tells sigaction() to use the sa_sigaction field, not sa_handler. */
	act.sa_flags = SA_SIGINFO;
 
	if (sigaction(SIGCHLD, &act, NULL) < 0)
		evm_log_return_system_err("sigaction() SIGCHLD\n");

	if (sigaction(SIGHUP, &act, NULL) < 0)
		evm_log_return_system_err("sigaction() SIGHUP\n");

	status = 0;
	return status;
}

static int messages_poll(struct evm_fds_struct *evm_fds, struct evm_sigpost_struct *evm_sigpost)
{
	int i, semVal;
	static int status = 0;

	if (status <= 0) {
		if (evm_fds->nfds <= FDS_TAB_SIZE) {
			/* THE ACTUAL BLOCKING POINT! */
			status = poll(evm_fds->ev_poll_fds, evm_fds->nfds, -1);
			if (status < 0) {
				if (errno == EINTR) {
					sem_getvalue(&semaphore, &semVal); /*after signal, check semaphore*/
					if (semVal > 0) {
						/* do something on SIGNALs here */
						evm_log_debug("Signal num %d received\n", evm_signum);
						if (evm_sigpost != NULL) {
							evm_sigpost->sigpost_handle(evm_signum, NULL);
						}
						evm_signum = 0;
						sem_trywait(&semaphore); /*after signal, lock semaphore*/
						sigfillset(&evm_sigmask);
						if (sigprocmask(SIG_UNBLOCK, &evm_sigmask, NULL) < 0) {
							evm_log_return_system_err("sigprocmask() SIG_UNBLOCK\n");
						}
					}
				} else {
					evm_log_return_system_err("poll(), nfds=%lu\n", evm_fds->nfds)
				}
				/* On EINTR do not report / return any error! */
				evm_log_debug("EINTR: nfds=%lu, FDS_TAB_SIZE=%d!\n", evm_fds->nfds, FDS_TAB_SIZE);
				status = -1;
				return status;
			}
		} else {
			evm_log_error("Number of polled FDs overflow: nfds=%lu, FDS_TAB_SIZE=%d!\n", evm_fds->nfds, FDS_TAB_SIZE);
			status = -1;
			return status;
		}
	}

	/* Pick first FD index with returned event. */
	for (i = 0; i < evm_fds->nfds; i++) {
		if (evm_fds->ev_poll_fds[i].revents != 0) {
			evm_fds->ev_poll_fds[i].revents = 0;
			status--;
			if (evm_fds->msg_ptrs[i] == NULL) {
				evm_log_debug("Polled FDs[%d] without allocated message buffers - Allocate now!\n", i);
				evm_fds->msg_ptrs[i] = (struct message_struct *)calloc(1, sizeof(struct message_struct));
				if (evm_fds->msg_ptrs[i] == NULL) {
					evm_log_return_system_err("calloc(): 1 times %zd bytes\n", sizeof(struct message_struct));
				}
				evm_fds->msg_ptrs[i]->fds_index = i;
			}
			evm_log_debug("Polled FDs[%d]!\n", i);
			return i;
		}
	}

	evm_log_error("No retured events - however poll returned %d!\n", status);
	status = -1;
	return status;
}

static int messages_receive(int fds_idx, struct evm_fds_struct *evm_fds, struct evm_link_struct *evm_linkage)
{
	int status = -1;

	/* Find registered receive call-back function. */
	if (evm_linkage == NULL)
		return -1;

	if (evm_fds->msg_receive[fds_idx] == NULL) {
		evm_log_error("Missing receive call-back function!\n");
		sleep(1);
		return -1;
	} else {
		/* Receive whatever message comes in. */
		status = evm_fds->msg_receive[fds_idx](evm_fds->ev_poll_fds[fds_idx].fd, evm_fds->msg_ptrs[fds_idx]);
		if (status < 0) {
			evm_log_error("Receive call-back function failed!\n");
		} else if (status == 0) {
			evm_log_debug("Receive call-back function - empty receive!\n");
		} else {
			evm_log_debug("Receive call-back function - buffered %d bytes!\n", evm_fds->msg_ptrs[fds_idx]->iov_buff.iov_len);
		}
	}

	return status;
}

static int messages_parse(int fds_idx, struct evm_fds_struct *evm_fds, struct evm_link_struct *evm_linkage)
{
	unsigned int ev_type = evm_fds->ev_type_fds[fds_idx];

	if (evm_linkage != NULL) {
		/* Add to parser all received data */
		if (evm_linkage[ev_type].ev_type_parse != NULL)
			return evm_linkage[ev_type].ev_type_parse((void *)evm_fds->msg_ptrs[fds_idx]);
	}

	/*no extra parser function - expected to be parsed already*/
	return 0;
}

void message_enqueue(struct message_struct *msg)
{
	if (evm_msg_queue.last_msg == NULL)
		evm_msg_queue.first_msg = msg;
	else
		evm_msg_queue.last_msg->next_msg = msg;

	evm_msg_queue.last_msg = msg;
	msg->next_msg = NULL;
}

struct message_struct * message_dequeue(void)
{
	struct message_struct *msg;

	msg = evm_msg_queue.first_msg;
	if (msg == NULL)
		return NULL;

	if (msg->next_msg == NULL) {
		evm_msg_queue.first_msg = NULL;
		evm_msg_queue.last_msg = NULL;
	} else
		evm_msg_queue.first_msg = msg->next_msg;

	msg->next_msg = NULL;
	return msg;
}

struct message_struct * messages_check(struct evm_init_struct *evm_init_ptr)
{
	int status = 0;
	int fds_index;

	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
		abort();
	}

	if (evm_init_ptr->evm_fds == NULL) {
		evm_log_error("FDs table empty - event machine init failed!\n");
		abort();
	}

	if (evm_init_ptr->evm_link == NULL) {
		evm_log_error("Event types linkage table empty - event machine init failed!\n");
		abort();
	}

	/* Poll the internal message queue first (NON-BLOCKING). */
	if (evm_msg_queue.first_msg != NULL) {
		return message_dequeue();
	}

	/* Poll the input (WAIT - THE ONLY BLOCKING POINT). */
	if ((fds_index = messages_poll(evm_init_ptr->evm_fds, evm_init_ptr->evm_sigpost)) < 0) {
		return NULL;
	}

	/* Receive any data. */
	if ((status = messages_receive(fds_index, evm_init_ptr->evm_fds, evm_init_ptr->evm_link)) < 0) {
		evm_log_debug("evm_receive() returned %d\n", status);
		return NULL;
	}

	/* Parse received data. */
	if ((status = messages_parse(fds_index, evm_init_ptr->evm_fds, evm_init_ptr->evm_link)) < 0) {
		evm_log_debug("evm_parse_message() returned %d\n", status);
		return NULL;
	}
	return evm_init_ptr->evm_fds->msg_ptrs[fds_index];
}

#if 1 /*orig*/
int message_concatenate(const void *buffer, size_t size, void *msgBuf)
{
	if ((buffer == NULL) || (msgBuf == NULL))
		return -1;

	strncat((char *)msgBuf, buffer, size);

	return 0;
}
#endif

