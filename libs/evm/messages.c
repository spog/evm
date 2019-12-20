/*
 * The EVM messages module
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

#ifndef EVM_FILE_messages_c
#define EVM_FILE_messages_c
#else
#error Preprocesor macro EVM_FILE_messages_c conflict!
#endif

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/eventfd.h>

#include "evm/libevm.h"

#include "messages.h"

#include "userlog/log_module.h"
EVMLOG_MODULE_INIT(EVM_MSGS, 1)

typedef struct message_queue message_queue_struct;
typedef struct message_hanger message_hanger_struct;

struct message_queue {
	message_hanger_struct *first_hanger;
	message_hanger_struct *last_hanger;
	evm_fd_struct *evmfd; /*internal message queue FD binding - eventfd()*/
	pthread_mutex_t mutex;
};

struct message_hanger {
	message_hanger_struct *next;
	message_hanger_struct *prev;
	evm_message_struct *msg; /*hangs of a hanger when linked in a chain - i.e.: in a message queue*/
};

static void messages_sighandler(int signum, siginfo_t *siginfo, void *context);
static int messages_sighandler_install(int signum);
static evm_fd_struct * messages_epoll(evm_init_struct *evm_init_ptr);
static int messages_receive(evm_fd_struct *evs_fd_ptr, evm_msgs_link_struct *evm_linkage);
static int messages_parse(evm_fd_struct *evs_fd_ptr, evm_msgs_link_struct *evm_linkage);
static evm_message_struct * message_dequeue(evm_init_struct *evm_init_ptr);
static int message_queue_evmfd_read(int efd, evm_message_struct *message);

static sigset_t messages_sigmask; /*actually a constant initialized via seigemptymask()*/
/* Thread-specific data related. */
static int thr_keys_not_created = 1;
static pthread_key_t thr_signum_key; /*initialized only once - not for each call to evm_init() within a process*/

static void messages_sighandler(int signum, siginfo_t *siginfo, void *context)
{
	int *thr_signum = (int *)pthread_getspecific(thr_signum_key);
	/* Save signum as thread-specific data. */
	*thr_signum = signum;
}

static int messages_sighandler_install(int signum)
{
	struct sigaction act;

	memset (&act, '\0', sizeof(act));
	/* Use the sa_sigaction field because the handles has two additional parameters */
	act.sa_sigaction = &messages_sighandler;
	/* The SA_SIGINFO flag tells sigaction() to use the sa_sigaction field, not sa_handler. */
	act.sa_flags = SA_SIGINFO;
 
	if (sigaction(signum, &act, NULL) < 0)
		evm_log_return_system_err("sigaction() for signum %d\n", signum);

	return 0;
}

int evm_messages_init(evm_init_struct *evm_init_ptr)
{
	int status = -1;
	void *ptr;
	sigset_t sigmask;
	evm_msgs_link_struct *evm_linkage;
	evm_sigpost_struct *evm_sigpost;

	evm_log_info("(entry)\n");
	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
		return status;
	}

	evm_linkage = evm_init_ptr->evm_msgs_link;
	if (evm_linkage == NULL) {
		evm_log_error("Event types linkage table empty - event machine init failed!\n");
		return status;
	}

	evm_sigpost = evm_init_ptr->evm_sigpost;
	if (evm_sigpost == NULL)
		evm_log_debug("Signal post-processing handler undefined!\n");

	/* Prepare thread-specific integer for thread related signal post-handling. */
	if (thr_keys_not_created) {
		if ((errno = pthread_key_create(&thr_signum_key, NULL)) != 0) {
			evm_log_return_system_err("pthread_key_create()\n");
		}
		thr_keys_not_created = 0;
	}
	if ((ptr = calloc(1, sizeof(int))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): thread-specific data\n");
		return status;
	}
	if ((errno = pthread_setspecific(thr_signum_key, ptr)) != 0) {
		evm_log_return_system_err("pthread_setspecific()\n");
	}

	/* Prepare module-local empty signal mask, used in epoll_pwait() to allow catching all signals there!*/!
	sigemptyset(&messages_sigmask);

	/* Unblock all signals, except HUP and CHLD, which are allowed to get caught only in epoll_pwait()! */
	evm_log_debug("Unblocking all signals, except SIGHUP and SIGCHLD\n");
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGHUP);
	sigaddset(&sigmask, SIGCHLD);
	if (pthread_sigmask(SIG_SETMASK, &sigmask, NULL) < 0) {
		evm_log_return_system_err("sigprocmask() SIG_SETMASK\n");
	}

	/* Install signal handler for SIGHUP and SIGCHLD. */
	evm_log_debug("Establishing handler for signal SIGHUP\n");
	if (messages_sighandler_install(SIGHUP) < 0) {
		evm_log_error("Failed to install SIGHUP signal handler!\n");
		return status;
	}
	evm_log_debug("Establishing handler for signal SIGCHLD\n");
	if (messages_sighandler_install(SIGCHLD) < 0) {
		evm_log_error("Failed to install SIGCHLD signal handler!\n");
		return status;
	}

	/* Setup internal message queue. */
	if ((ptr = calloc(1, sizeof(message_queue_struct))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): internal message queue\n");
		return status;
	}
	evm_init_ptr->msg_queue = ptr;
	pthread_mutex_init(&((message_queue_struct *)ptr)->mutex, NULL);
	pthread_mutex_unlock(&((message_queue_struct *)ptr)->mutex);

	/* Setup EPOLL infrastructure. */
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

int evm_messages_queue_fd_init(evm_init_struct *evm_init_ptr)
{
	int status = -1;
	evm_fd_struct *ptr;

	evm_log_info("(entry) evm_init_ptr=%p\n", evm_init_ptr);
	/* Prepare internal message queue FD for EPOLL to operate over internal linked list. */
	if ((ptr = (evm_fd_struct *)calloc(1, sizeof(evm_fd_struct))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): internal message queue evm FD\n");
		return status;
	}
	((message_queue_struct *)(evm_init_ptr->msg_queue))->evmfd = ptr;

	if ((ptr->fd = eventfd(0, EFD_CLOEXEC)) < 0)
		evm_log_return_system_err("eventfd()\n");

	ptr->ev_epoll.events = EPOLLIN;
	ptr->ev_epoll.data.ptr = (void *)ptr /*our own address*/;
	ptr->msg_receive = message_queue_evmfd_read;
//	ptr->msg_send = NULL;
	if (evm_message_fd_add(evm_init_ptr, ptr) < 0) {
		return status;
	}

	return 0;
}

static evm_fd_struct * messages_epoll(evm_init_struct *evm_init_ptr)
{
	int *thr_signum;
	evm_fd_struct *evs_fd_ptr;

	evm_log_info("(entry)\n");
	if (evm_init_ptr->epoll_nfds <= 0) {
//not sure, if necessary:		bzero((void *)evm_init_ptr->epoll_events, sizeof(struct epoll_event) * evm_init_ptr->epoll_max_events);
		evm_log_debug("evm_init_ptr->epoll_max_events: %d, sizeof(struct epoll_event): %zu\n", evm_init_ptr->epoll_max_events, sizeof(struct epoll_event));
		/* THE ACTUAL BLOCKING POINT! */
		evm_init_ptr->epoll_nfds = epoll_pwait(evm_init_ptr->epollfd, evm_init_ptr->epoll_events, evm_init_ptr->epoll_max_events, evm_init_ptr->epoll_timeout, &messages_sigmask);
		if (evm_init_ptr->epoll_nfds < 0) {
			if (errno == EINTR) {
				evm_log_debug("epoll_wait(): EINTR\n");
				thr_signum = (int *)pthread_getspecific(thr_signum_key);
				if (*thr_signum > 0) {
					/* do something on SIGNALs here */
					evm_log_debug("Signal %d handled. Call signal post-handler, if provided!\n", *thr_signum);
					if (evm_init_ptr->evm_sigpost != NULL) {
						evm_init_ptr->evm_sigpost->sigpost_handle(*thr_signum, NULL);
					}
					*thr_signum = 0;
				}
			} else {
				evm_log_system_error("epoll()\n")
				return NULL;
			}
			/* On EINTR do not report error! */
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

static int messages_receive(evm_fd_struct *evs_fd_ptr, evm_msgs_link_struct *evm_linkage)
{
	int status = -1;

	evm_log_info("(entry) evs_fd_ptr=%p\n", evs_fd_ptr);
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
			evm_log_debug("Receive call-back function - empty receive or internal queue receive!\n");
		} else {
			evm_log_debug("Receive call-back function - buffered %zd bytes!\n", evs_fd_ptr->msg_ptr->iov_buff.iov_len);
		}
	}

	return status;
}

static int messages_parse(evm_fd_struct *evs_fd_ptr, evm_msgs_link_struct *evm_linkage)
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

int evm_message_enqueue(evm_init_struct *evm_init_ptr, evm_message_struct *msg)
{
	int status = -1;
	message_queue_struct *msg_queue = (message_queue_struct *)evm_init_ptr->msg_queue;
	pthread_mutex_t *mtx = &msg_queue->mutex;
	message_hanger_struct *msg_hanger;

	evm_log_info("(entry)\n");
	pthread_mutex_lock(mtx);
	if ((msg_hanger = (message_hanger_struct *)calloc(1, sizeof(message_hanger_struct))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): message hanger\n");
		pthread_mutex_unlock(mtx);
		return status;
	}
	msg_hanger->msg = msg;
	if (msg_queue->last_hanger == NULL)
		msg_queue->first_hanger = msg_hanger;
	else
		msg_queue->last_hanger->next = msg_hanger;

	msg_queue->last_hanger = msg_hanger;
	msg_hanger->next = NULL;
	pthread_mutex_unlock(mtx);
	return 0;
}

static evm_message_struct * message_dequeue(evm_init_struct *evm_init_ptr)
{
	evm_message_struct *msg;
	message_queue_struct *msg_queue = (message_queue_struct *)evm_init_ptr->msg_queue;
	pthread_mutex_t *mtx = &msg_queue->mutex;
	message_hanger_struct *msg_hanger;

	evm_log_info("(entry)\n");
	pthread_mutex_lock(mtx);
	msg_hanger = msg_queue->first_hanger;
	if (msg_hanger == NULL) {
		pthread_mutex_unlock(mtx);
		return NULL;
	}

	if (msg_hanger->next == NULL) {
		msg_queue->first_hanger = NULL;
		msg_queue->last_hanger = NULL;
	} else
		msg_queue->first_hanger = msg_hanger->next;

	msg = msg_hanger->msg;
	free(msg_hanger);
	msg_hanger = NULL;
	msg->evm_ptr = evm_init_ptr;
	pthread_mutex_unlock(mtx);
	return msg;
}

evm_message_struct * evm_messages_check(evm_init_struct *evm_init_ptr)
{
	int status = 0;
	evm_fd_struct *evs_fd_ptr = NULL;
	message_queue_struct *msg_queue = (message_queue_struct *)evm_init_ptr->msg_queue;

	evm_log_info("(entry) queue->first=%p\n", msg_queue->first_hanger);
	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
		abort();
	}

	if (evm_init_ptr->evm_msgs_link == NULL) {
		evm_log_error("Message event types linkage table empty - event machine init failed!\n");
		abort();
	}

	/* Poll the internal message queue first (NON-BLOCKING). */
#if 1 /*orig*/
	if (msg_queue->first_hanger != NULL) {
		return message_dequeue(evm_init_ptr);
	}
#endif

	/* EPOLL the input (WAIT - THE ONLY BLOCKING POINT). */
	if ((evs_fd_ptr = messages_epoll(evm_init_ptr)) == NULL) {
		return NULL;
	}

	/* Receive any data. */
	if ((status = messages_receive(evs_fd_ptr, evm_init_ptr->evm_msgs_link)) < 0) {
		evm_log_debug("messages_receive() returned %d\n", status);
		return NULL;
	}

	if (
		(evs_fd_ptr == ((message_queue_struct *)evm_init_ptr->msg_queue)->evmfd) ||
		(evs_fd_ptr == ((message_queue_struct *)evm_init_ptr->tmr_queue)->evmfd)
	) {
		evm_log_debug("Internal queue receive triggered!\n");
#if 0 /*test*/
		if (msg_queue->first_hanger != NULL) {
			return message_dequeue(evm_init_ptr);
		}
#endif
		return NULL;
	}
	/* Parse received data. */
	if ((status = messages_parse(evs_fd_ptr, evm_init_ptr->evm_msgs_link)) < 0) {
		evm_log_debug("evm_parse_message() returned %d\n", status);
		return NULL;
	}

	if (evs_fd_ptr != NULL)
		if (evs_fd_ptr->msg_ptr != NULL) {
			/* A new message has been decoded! */
			evs_fd_ptr->msg_ptr->evm_ptr = evm_init_ptr;
			return evs_fd_ptr->msg_ptr;
		}

	return NULL;
}

static int message_queue_evmfd_read(int efd, evm_message_struct *message)
{
	uint64_t u;
	ssize_t s;

	evm_log_info("(cb entry) eventfd=%d, message=%p\n", efd, message);
	if ((s = read(efd, &u, sizeof(uint64_t))) != sizeof(uint64_t))
		evm_log_system_error("read(): message_queue_evmfd_read\n");

	message->iov_buff.iov_len = 0;
	message->iov_buff.iov_base = NULL;

	evm_log_debug("buffered=%zd, received data ptr: %p\n", message->iov_buff.iov_len, message->iov_buff.iov_base);
	return 0;
}

int evm_message_call(evm_init_struct *evm_init_ptr, evm_message_struct *msg)
{
	uint64_t u;
	ssize_t s;

	evm_log_info("(entry) evm_init_ptr=%p, msg=%p\n", evm_init_ptr, msg);
	if (evm_init_ptr != NULL) {
		if (evm_message_enqueue(evm_init_ptr, msg) != 0) {
			evm_log_error("Message enqueuing failed!\n");
			return -1;
		}
		return 0;
	}
	return -1;
}

int evm_message_pass(evm_init_struct *evm_init_ptr, evm_message_struct *msg)
{
	uint64_t u;
	ssize_t s;

	evm_log_info("(entry) evm_init_ptr=%p, msg=%p\n", evm_init_ptr, msg);
	if (evm_init_ptr != NULL) {
		if (evm_message_enqueue(evm_init_ptr, msg) != 0) {
			evm_log_error("Message enqueuing failed!\n");
			return -1;
		}

		u = 1;
		if ((s = write(((message_queue_struct *)evm_init_ptr->msg_queue)->evmfd->fd, &u, sizeof(uint64_t))) != sizeof(uint64_t)) {
			evm_log_system_error("write(): evm_message_pass\n");
			if (message_dequeue(evm_init_ptr) == NULL) {
				evm_log_error("Message dequeuing failed!\n");
			}
			return -1;
		}
		return 0;
	}
	return -1;
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
	evm_log_debug("evm_init_ptr->epollfd: %d, evm_fd_ptr->fd: %d\n", evm_init_ptr->epollfd, evm_fd_ptr->fd);
	if (epoll_ctl(evm_init_ptr->epollfd, EPOLL_CTL_ADD, evm_fd_ptr->fd, &evm_fd_ptr->ev_epoll) < 0) {
		evm_log_return_system_err("epoll_ctl()\n");
	}

	return 0;
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

