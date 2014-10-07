/*
 *  Copyright (C) 2012  Samo Pogacnik
 */

/*
 * The EVM messages module
 */
#ifndef evm_messages_c
#define evm_messages_c
#else
#error Preprocesor macro evm_messages_c conflict!
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

int evm_messages_init(evm_init_struct *evm_init_ptr)
{
	int status = -1;
	struct sigaction act;
	evm_link_struct *evm_linkage;
	evm_sigpost_struct *evm_sigpost;

	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
		return status;
	}

	evm_linkage = evm_init_ptr->evm_link;
	if (evm_linkage == NULL) {
		evm_log_error("Event types linkage table empty - event machine init failed!\n");
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

	if ((evm_init_ptr->epollfd = epoll_create1(0)) < 0)
		evm_log_return_system_err("epoll_create1()\n");

	if (evm_init_ptr->epoll_max_events <= 0) {
		evm_log_error("epoll_max_events = %d (need to be positive number) - event machine init failed!\n", evm_init_ptr->epoll_max_events);
		return status;
	}

	evm_init_ptr->epoll_events = (struct epoll_event *)calloc(evm_init_ptr->epoll_max_events, sizeof(struct epoll_event));
	if (evm_init_ptr->epoll_events == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): %d times %zd bytes\n", evm_init_ptr->epoll_max_events, sizeof(struct epoll_event));
		return status;
	}

	evm_init_ptr->epoll_nfds = -1;
	return 0;
}

static evm_fd_struct * messages_epoll(evm_init_struct *evm_init_ptr)
{
	int semVal;
	evm_fd_struct *evs_fd_ptr;

	evm_log_info("(entry)\n");
	if (evm_init_ptr->epoll_nfds <= 0) {
//not sure, if necessary:		bzero((void *)evm_init_ptr->epoll_events, sizeof(struct epoll_event) * evm_init_ptr->epoll_max_events);
		evm_log_debug("evm_init_ptr->epoll_max_events: %d, sizeof(struct epoll_event): %zu\n", evm_init_ptr->epoll_max_events, sizeof(struct epoll_event));
		/* THE ACTUAL BLOCKING POINT! */
		evm_init_ptr->epoll_nfds = epoll_wait(evm_init_ptr->epollfd, evm_init_ptr->epoll_events, evm_init_ptr->epoll_max_events, -1);
		if (evm_init_ptr->epoll_nfds < 0) {
			if (errno == EINTR) {
				sem_getvalue(&semaphore, &semVal); /*after signal, check semaphore*/
				if (semVal > 0) {
					/* do something on SIGNALs here */
					evm_log_debug("Signal num %d received\n", evm_signum);
					if (evm_init_ptr->evm_sigpost != NULL) {
						evm_init_ptr->evm_sigpost->sigpost_handle(evm_signum, NULL);
					}
					evm_signum = 0;
					sem_trywait(&semaphore); /*after signal, lock semaphore*/
					sigfillset(&evm_sigmask);
					if (sigprocmask(SIG_UNBLOCK, &evm_sigmask, NULL) < 0) {
						evm_log_system_error("sigprocmask() SIG_UNBLOCK\n");
						return NULL;
					}
				}
			} else {
				evm_log_system_error("epoll()\n")
				return NULL;
			}
			/* On EINTR do not report / return any error! */
			evm_log_debug("epoll_wait(): EINTR\n");
			return NULL;
		}
	}

	evm_log_debug("Epoll returned %d FDs ready!\n", evm_init_ptr->epoll_nfds);
	if (evm_init_ptr->epoll_nfds == 0) {
		return NULL;
	}

	/* Pick the last events element with returned event. */
	evm_init_ptr->epoll_nfds--;
	evs_fd_ptr = (evm_fd_struct *)evm_init_ptr->epoll_events[evm_init_ptr->epoll_nfds].data.ptr;
	evm_log_debug("evs_fd_ptr: %p\n", evs_fd_ptr);
	if (
		(evm_init_ptr->epoll_events[evm_init_ptr->epoll_nfds].events != 0) &&
		((evm_init_ptr->epoll_events[evm_init_ptr->epoll_nfds].events & EPOLLERR) == 0) &&
		((evm_init_ptr->epoll_events[evm_init_ptr->epoll_nfds].events & EPOLLHUP) == 0)
	) {
		if (evs_fd_ptr->msg_ptr == NULL) {
			evm_log_debug("Polled FD without allocated message buffers - Allocate now!\n");
			evs_fd_ptr->msg_ptr = (evm_message_struct *)calloc(1, sizeof(evm_message_struct));
			if (evs_fd_ptr->msg_ptr == NULL) {
				errno = ENOMEM;
				evm_log_system_error("calloc(): 1 times %zd bytes\n", sizeof(evm_message_struct));
				return NULL;
			}
		}
		evm_log_debug("Polled FD (%d) (events mask 0x%x)!\n", evs_fd_ptr->fd, evm_init_ptr->epoll_events[evm_init_ptr->epoll_nfds].events);
	} else {
		evm_log_debug("Polled FD (%d) with ERROR (events mask 0x%x)!\n", evs_fd_ptr->fd, evm_init_ptr->epoll_events[evm_init_ptr->epoll_nfds].events);
	}

	evm_log_info("(exit)\n");
	return evs_fd_ptr;
}

static int messages_receive(evm_fd_struct *evs_fd_ptr, evm_link_struct *evm_linkage)
{
	int status = -1;

	/* Find registered receive call-back function. */
	if (evm_linkage == NULL)
		return -1;

	if (evs_fd_ptr->msg_receive == NULL) {
		evm_log_error("Missing receive call-back function!\n");
		sleep(1);
		return -1;
	} else {
		/* Receive whatever message comes in. */
		status = evs_fd_ptr->msg_receive(evs_fd_ptr->fd, evs_fd_ptr->msg_ptr);
		if (status < 0) {
			evm_log_error("Receive call-back function failed!\n");
		} else if (status == 0) {
			evm_log_debug("Receive call-back function - empty receive!\n");
		} else {
			evm_log_debug("Receive call-back function - buffered %d bytes!\n", evs_fd_ptr->msg_ptr->iov_buff.iov_len);
		}
	}

	return status;
}

static int messages_parse(evm_fd_struct *evs_fd_ptr, evm_link_struct *evm_linkage)
{
	unsigned int ev_type = evs_fd_ptr->ev_type;

	if (evm_linkage != NULL) {
		/* Add to parser all received data */
		if (evm_linkage[ev_type].ev_type_parse != NULL)
			return evm_linkage[ev_type].ev_type_parse((void *)evs_fd_ptr->msg_ptr);
	}

	/*no extra parser function - expected to be parsed already*/
	return 0;
}

void evm_message_enqueue(evm_message_struct *msg)
{
	if (evm_msg_queue.last_msg == NULL)
		evm_msg_queue.first_msg = msg;
	else
		evm_msg_queue.last_msg->next_msg = msg;

	evm_msg_queue.last_msg = msg;
	msg->next_msg = NULL;
}

static evm_message_struct * message_dequeue(void)
{
	evm_message_struct *msg;

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

evm_message_struct * evm_messages_check(evm_init_struct *evm_init_ptr)
{
	int status = 0;
	evm_fd_struct *evs_fd_ptr;

	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
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

	/* EPOLL the input (WAIT - THE ONLY BLOCKING POINT). */
	if ((evs_fd_ptr = messages_epoll(evm_init_ptr)) == NULL) {
		return NULL;
	}

	/* Receive any data. */
	if ((status = messages_receive(evs_fd_ptr, evm_init_ptr->evm_link)) < 0) {
		evm_log_debug("messages_receive() returned %d\n", status);
		return NULL;
	}

	/* Parse received data. */
	if ((status = messages_parse(evs_fd_ptr, evm_init_ptr->evm_link)) < 0) {
		evm_log_debug("evm_parse_message() returned %d\n", status);
		return NULL;
	}
	return evs_fd_ptr->msg_ptr;
}

#if 1 /*orig*/
int evm_message_concatenate(const void *buffer, size_t size, void *msgBuf)
{
	if ((buffer == NULL) || (msgBuf == NULL))
		return -1;

	strncat((char *)msgBuf, buffer, size);

	return 0;
}
#endif

