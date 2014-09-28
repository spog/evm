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

static msgs_epoll_max_events = 1;
static struct epoll_event *msgs_epoll_events;

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

	if ((evm_init_ptr->evm_epollfd = epoll_create1(0)) < 0)
		evm_log_return_system_err("epoll_create1()\n");

	if (evm_init_ptr->evm_epoll_max_events > 0)
		msgs_epoll_max_events = evm_init_ptr->evm_epoll_max_events;

	msgs_epoll_events = (struct epoll_event *)calloc(msgs_epoll_max_events, sizeof(struct epoll_event));
	if (msgs_epoll_events == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): %d times %zd bytes\n", msgs_epoll_max_events, sizeof(struct epoll_event));
		return status;
	}

	return 0;
}

static struct evm_fd_struct * messages_epoll(int evm_epollfd, struct evm_sigpost_struct *evm_sigpost)
{
	int semVal;
	struct evm_fd_struct *evs_fd_ptr;
	static int nfds = -1;

	evm_log_info("(entry)\n");
	if (nfds <= 0) {
//not sure, if necessary:		bzero((void *)msgs_epoll_events, sizeof(msgs_epoll_events) * msgs_epoll_max_events);
		evm_log_debug("msgs_epoll_max_events: %d, sizeof(struct epoll_event): %zu\n", msgs_epoll_max_events, sizeof(struct epoll_event));
		/* THE ACTUAL BLOCKING POINT! */
		nfds = epoll_wait(evm_epollfd, msgs_epoll_events, msgs_epoll_max_events, -1);
		if (nfds < 0) {
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

	evm_log_debug("Epoll returned %d FDs ready!\n", nfds);
	if (nfds == 0) {
		return NULL;
	}

	/* Pick the last events element with returned event. */
	nfds--;
	evs_fd_ptr = (struct evm_fd_struct *)msgs_epoll_events[nfds].data.ptr;
	evm_log_debug("evs_fd_ptr: %p\n", evs_fd_ptr);
	if (
		(msgs_epoll_events[nfds].events != 0) &&
		((msgs_epoll_events[nfds].events & EPOLLERR) == 0) &&
		((msgs_epoll_events[nfds].events & EPOLLHUP) == 0)
	) {
		if (evs_fd_ptr->msg_ptr == NULL) {
			evm_log_debug("Polled FD without allocated message buffers - Allocate now!\n");
			evs_fd_ptr->msg_ptr = (struct message_struct *)calloc(1, sizeof(struct message_struct));
			if (evs_fd_ptr->msg_ptr == NULL) {
				errno = ENOMEM;
				evm_log_system_error("calloc(): 1 times %zd bytes\n", sizeof(struct message_struct));
				return NULL;
			}
		}
		evm_log_debug("Polled FD (%d) (events mask 0x%x)!\n", evs_fd_ptr->fd, msgs_epoll_events[nfds].events);
	} else {
		evm_log_debug("Polled FD (%d) with ERROR (events mask 0x%x)!\n", evs_fd_ptr->fd, msgs_epoll_events[nfds].events);
	}

	evm_log_info("(exit)\n");
	return evs_fd_ptr;
}

static int messages_receive(struct evm_fd_struct *evs_fd_ptr, struct evm_link_struct *evm_linkage)
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

static int messages_parse(struct evm_fd_struct *evs_fd_ptr, struct evm_link_struct *evm_linkage)
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
	struct evm_fd_struct *evs_fd_ptr;

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
	if ((evs_fd_ptr = messages_epoll(evm_init_ptr->evm_epollfd, evm_init_ptr->evm_sigpost)) == NULL) {
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
int message_concatenate(const void *buffer, size_t size, void *msgBuf)
{
	if ((buffer == NULL) || (msgBuf == NULL))
		return -1;

	strncat((char *)msgBuf, buffer, size);

	return 0;
}
#endif

