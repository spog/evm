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

#include "evm.h"
#include "messages.h"

#include "userlog/log_module.h"
EVMLOG_MODULE_INIT(EVM_MSGS, 1)

typedef struct msgs_queue msgs_queue_struct;
typedef struct msg_hanger msg_hanger_struct;

struct msgs_queue {
	msg_hanger_struct *first_hanger;
	msg_hanger_struct *last_hanger;
//samo - orig:	evm_fd_struct *evmfd; /*internal message queue FD binding - eventfd()*/
	pthread_mutex_t access_mutex;
}; /*msgs_queue_struct*/

struct msg_hanger {
	msg_hanger_struct *next;
	msg_hanger_struct *prev;
	evm_message_struct *msg; /*hangs of a hanger when linked in a chain - i.e.: in a message queue*/
}; /*msg_hanger_struct*/

static sigset_t msgs_sigmask; /*actually a constant initialized via sigemptymask()*/
/* Thread-specific data related. */
static int msg_thr_keys_created = EVM_FALSE;
static pthread_key_t msg_thr_signum_key; /*initialized only once - not for each call to evm_init() within a process*/

static void msgs_sighandler(int signum, siginfo_t *siginfo, void *context);
static int msgs_sighandler_install(int signum);
#if 0 /*samo - orig*/
static int messages_receive(evm_fd_struct *evs_fd_ptr);
static int messages_parse(evm_fd_struct *evs_fd_ptr);
static int message_queue_evmfd_read(int efd, evm_message_struct *message);
#endif
static int msg_enqueue(evm_init_struct *evm_init_ptr, evm_message_struct *msg);
static evm_message_struct * msg_dequeue(evm_init_struct *evm_init_ptr);

static void msgs_sighandler(int signum, siginfo_t *siginfo, void *context)
{
	int *thr_signum = (int *)pthread_getspecific(msg_thr_signum_key);
	/* Save signum as thread-specific data. */
	*thr_signum = signum;
}

static int msgs_sighandler_install(int signum)
{
	struct sigaction act;

	memset (&act, '\0', sizeof(act));
	/* Use the sa_sigaction field because the handle has two additional parameters */
	act.sa_sigaction = &msgs_sighandler;
	/* The SA_SIGINFO flag tells sigaction() to use the sa_sigaction field, not sa_handler. */
	act.sa_flags = SA_SIGINFO;
 
	if (sigaction(signum, &act, NULL) < 0)
		evm_log_return_system_err("sigaction() for signum %d\n", signum);

	return 0;
}

int messages_init(evm_init_struct *evm_init_ptr)
{
	int status = -1;
	void *ptr;
	sigset_t sigmask;
	evm_sigpost_struct *evm_sigpost;

	evm_log_info("(entry)\n");
	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
		return status;
	}

	evm_sigpost = evm_init_ptr->evm_sigpost;
	if (evm_sigpost == NULL)
		evm_log_debug("Signal post-processing handler undefined!\n");

	/* Prepare thread-specific integer for thread related signal post-handling. */
	if (!msg_thr_keys_created) {
		if ((errno = pthread_key_create(&msg_thr_signum_key, NULL)) != 0) {
			evm_log_return_system_err("pthread_key_create()\n");
		}
		msg_thr_keys_created = EVM_TRUE;
	}
	if ((ptr = calloc(1, sizeof(int))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): thread-specific data\n");
		return status;
	}
	if ((errno = pthread_setspecific(msg_thr_signum_key, ptr)) != 0) {
		evm_log_return_system_err("pthread_setspecific()\n");
	}

	/* Prepare module-local empty signal mask, used in epoll_pwait() to allow catching all signals there!*/!
	sigemptyset(&msgs_sigmask);

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
	if (msgs_sighandler_install(SIGHUP) < 0) {
		evm_log_error("Failed to install SIGHUP signal handler!\n");
		return status;
	}
	evm_log_debug("Establishing handler for signal SIGCHLD\n");
	if (msgs_sighandler_install(SIGCHLD) < 0) {
		evm_log_error("Failed to install SIGCHLD signal handler!\n");
		return status;
	}

	/* Setup internal message queue. */
	if ((ptr = calloc(1, sizeof(msgs_queue_struct))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): internal message queue\n");
		return status;
	}
	evm_init_ptr->msgs_queue = ptr;
	pthread_mutex_init(&((msgs_queue_struct *)ptr)->access_mutex, NULL);
	pthread_mutex_unlock(&((msgs_queue_struct *)ptr)->access_mutex);

	return 0;
}

#if 0 /*samo - orig*/
static int messages_receive(evm_fd_struct *evs_fd_ptr)
{
	int status = -1;

	evm_log_info("(entry) evs_fd_ptr=%p\n", evs_fd_ptr);
	/* Find registered receive call-back function. */
	if (evs_fd_ptr == NULL)
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

static int messages_parse(evm_fd_struct *evs_fd_ptr)
{
	evm_evtypes_list_struct *evtype_ptr;

	if (evs_fd_ptr == NULL)
		return -1;

	if (evs_fd_ptr->evtype_ptr == NULL)
		return -1;

	evtype_ptr = evs_fd_ptr->evtype_ptr;

	/* Parse all received data */
	if (evtype_ptr->ev_parse != NULL)
		return evtype_ptr->ev_parse((void *)evs_fd_ptr->msg_ptr);

	/*no extra parser function - expected to be parsed already*/
	return 0;
}
#endif

static int msg_enqueue(evm_init_struct *evm_init_ptr, evm_message_struct *msg)
{
	int status = -1;
	msgs_queue_struct *msgs_queue = (msgs_queue_struct *)evm_init_ptr->msgs_queue;
	sem_t *bsem = &evm_init_ptr->blocking_sem;
	pthread_mutex_t *amtx = &msgs_queue->access_mutex;
	msg_hanger_struct *msg_hanger;

	evm_log_info("(entry)\n");
	pthread_mutex_lock(amtx);
	if ((msg_hanger = (msg_hanger_struct *)calloc(1, sizeof(msg_hanger_struct))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): message hanger\n");
		pthread_mutex_unlock(amtx);
		return status;
	}
	msg_hanger->msg = msg;
	if (msgs_queue->last_hanger == NULL)
		msgs_queue->first_hanger = msg_hanger;
	else
		msgs_queue->last_hanger->next = msg_hanger;

	msgs_queue->last_hanger = msg_hanger;
	msg_hanger->next = NULL;
	pthread_mutex_unlock(amtx);
	evm_log_info("Post blocking semaphore (UNBLOCK)\n");
	sem_post(bsem);
	return 0;
}

static evm_message_struct * msg_dequeue(evm_init_struct *evm_init_ptr)
{
	int rv = 0;
	evm_message_struct *msg;
	msgs_queue_struct *msgs_queue = (msgs_queue_struct *)evm_init_ptr->msgs_queue;
	sem_t *bsem = &evm_init_ptr->blocking_sem;
	pthread_mutex_t *amtx = &msgs_queue->access_mutex;
	msg_hanger_struct *msg_hanger;

	evm_log_info("(entry)\n");

	evm_log_info("Wait blocking semaphore (BLOCK, IF LOCKED)\n");
	sem_wait(bsem);
	pthread_mutex_lock(amtx);
	msg_hanger = msgs_queue->first_hanger;
	if (msg_hanger == NULL) {
		pthread_mutex_unlock(amtx);
		return NULL;
	}

	if (msg_hanger->next == NULL) {
		msgs_queue->first_hanger = NULL;
		msgs_queue->last_hanger = NULL;
	} else
		msgs_queue->first_hanger = msg_hanger->next;

	msg = msg_hanger->msg;
	free(msg_hanger);
	msg_hanger = NULL;
	pthread_mutex_unlock(amtx);
	return msg;
}

evm_message_struct * messages_check(evm_init_struct *evm_init_ptr)
{
	int status = 0;
//samo - orig:	evm_fd_struct *evs_fd_ptr = NULL;
	msgs_queue_struct *msgs_queue = (msgs_queue_struct *)evm_init_ptr->msgs_queue;

	evm_log_info("(entry) queue->first=%p\n", msgs_queue->first_hanger);
	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
		abort();
	}

	/* Poll the internal message queue first (THE ONLY POTENTIALLY BLOCKING POINT). */
	return msg_dequeue(evm_init_ptr);

#if 0 /*samo - orig*/
	/* Receive any data. */
	if ((status = messages_receive(evs_fd_ptr)) < 0) {
		evm_log_debug("messages_receive() returned %d\n", status);
		return NULL;
	}

	if (evs_fd_ptr == ((msgs_queue_struct *)evm_init_ptr->msgs_queue)->evmfd) {
		evm_log_debug("Internal queue receive triggered!\n");
#if 0 /*test*/
		if (msgs_queue->first_hanger != NULL) {
			return msg_dequeue(evm_init_ptr);
		}
#endif
		return NULL;
	}
	/* Parse received data. */
	if ((status = messages_parse(evs_fd_ptr)) < 0) {
		evm_log_debug("evm_parse_message() returned %d\n", status);
		return NULL;
	}

	if (evs_fd_ptr != NULL)
		if (evs_fd_ptr->msg_ptr != NULL) {
			/* A new message has been decoded! */
			return evs_fd_ptr->msg_ptr;
		}
#endif

	return NULL;
}

#if 0 /*samo - orig*/
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
#endif

int evm_message_pass(evm_init_struct *evm_init_ptr, evm_message_struct *msg)
{
	evm_log_info("(entry) evm_init_ptr=%p, msg=%p\n", evm_init_ptr, msg);
	if (evm_init_ptr != NULL) {
		if (msg_enqueue(evm_init_ptr, msg) != 0) {
			evm_log_error("Message enqueuing failed!\n");
			return -1;
		}
		return 0;
	}
	return -1;
}

#if 1 /*orig*/
int evm_message_concatenate(const void *buffer, size_t size, void *msgBuf)
{
	evm_log_info("(entry)\n");

	if ((buffer == NULL) || (msgBuf == NULL))
		return -1;

	strncat((char *)msgBuf, buffer, size);

	return 0;
}
#endif

evm_evtypes_list_struct * evm_msgtype_add(evm_init_struct *evm_init_ptr, int msg_type)
{
	evm_evtypes_list_struct *new, *tmp;
	evm_log_info("(entry)\n");

	if (evm_init_ptr == NULL)
		return NULL;

	tmp = evm_init_ptr->msgtypes_first;
	while (tmp != NULL) {
		if (msg_type == tmp->ev_type)
			return tmp; /* returns the same as evm_msgtype_get() */
		if (tmp->next == NULL)
			break;
		tmp = tmp->next;
	}

	if ((new = calloc(1, sizeof(evm_evtypes_list_struct))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): evm event types list\n");
		return NULL;
	}
	new->evm_ptr = evm_init_ptr;
	new->ev_type = msg_type;
	new->ev_parse = NULL;
	new->evids_first = NULL;
	new->prev = tmp;
	new->next = NULL;
	if (tmp != NULL)
		tmp->next = new;
	else
		evm_init_ptr->msgtypes_first = new;
	return new;
}

evm_evtypes_list_struct * evm_msgtype_get(evm_init_struct *evm_init_ptr, int msg_type)
{
	evm_evtypes_list_struct *tmp = NULL;
	evm_log_info("(entry)\n");

	if (evm_init_ptr == NULL)
		return NULL;

	tmp = evm_init_ptr->msgtypes_first;
	while (tmp != NULL) {
		evm_log_debug("evm_msgtype_get() tmp=%p,tmp->ev_type=%d, msg_type=%d\n", (void *)tmp, tmp->ev_type, msg_type);
		if (msg_type == tmp->ev_type)
			return tmp;
		tmp = tmp->next;
	}

	return NULL;
}

evm_evtypes_list_struct * evm_msgtype_del(evm_init_struct *evm_init_ptr, int msg_type)
{
	evm_evtypes_list_struct *tmp = NULL;
	evm_log_info("(entry)\n");

	if (evm_init_ptr == NULL)
		return NULL;

	tmp = evm_init_ptr->msgtypes_first;
	while (tmp != NULL) {
		if (msg_type == tmp->ev_type)
			break;
		tmp = tmp->next;
	}

	if (tmp != NULL) {
		tmp->prev->next = tmp->next;
		if (tmp->next != NULL)
			tmp->next->prev = tmp->prev;
		free(tmp);
	}
	return tmp;
}

int evm_msgtype_parse_cb_set(evm_evtypes_list_struct *msgtype_ptr, int (*ev_parse)(void *ev_ptr))
{
	int rv = 0;
	evm_log_info("(entry)\n");

	if (msgtype_ptr == NULL)
		return -1;

	if (ev_parse == NULL)
		return -1;

	if (rv == 0) {
		msgtype_ptr->ev_parse = ev_parse;
	}
	return rv;

}

evm_evids_list_struct * evm_msgid_add(evm_evtypes_list_struct *msgtype_ptr, int msg_id)
{
	evm_evids_list_struct *new, *tmp;
	evm_log_info("(entry)\n");

	if (msgtype_ptr == NULL)
		return NULL;

	tmp = msgtype_ptr->evids_first;
	while (tmp != NULL) {
		if (msg_id == tmp->ev_id)
			return tmp; /* returns the same as evm_msgid_get() */
		if (tmp->next == NULL)
			break;
		tmp = tmp->next;
	}

	if ((new = calloc(1, sizeof(evm_evids_list_struct))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): evm event ids list\n");
		return NULL;
	}
	new->evm_ptr = NULL;
	new->evtype_ptr = msgtype_ptr;
	new->ev_id = msg_id;
	new->ev_prepare = NULL;
	new->ev_handle = NULL;
	new->ev_finalize = NULL;
	new->prev = tmp;
	new->next = NULL;
	if (tmp != NULL)
		tmp->next = new;
	else
		msgtype_ptr->evids_first = new;
	return new;
}

evm_evids_list_struct * evm_msgid_get(evm_evtypes_list_struct *msgtype_ptr, int msg_id)
{
	evm_evids_list_struct *tmp = NULL;
	evm_log_info("(entry)\n");

	if (msgtype_ptr == NULL)
		return NULL;

	tmp = msgtype_ptr->evids_first;
	while (tmp != NULL) {
		evm_log_debug("evm_msgid_get() tmp=%p,tmp->ev_id=%d, msg_id=%d\n", (void *)tmp, tmp->ev_id, msg_id);
		if (msg_id == tmp->ev_id)
			return tmp;
		tmp = tmp->next;
	}

	return NULL;
}

evm_evids_list_struct * evm_msgid_del(evm_evtypes_list_struct *msgtype_ptr, int msg_id)
{
	evm_evids_list_struct *tmp = NULL;
	evm_log_info("(entry)\n");

	if (msgtype_ptr == NULL)
		return NULL;

	tmp = msgtype_ptr->evids_first;
	while (tmp != NULL) {
		if (msg_id == tmp->ev_id)
			break;
		tmp = tmp->next;
	}

	if (tmp != NULL) {
		tmp->prev->next = tmp->next;
		if (tmp->next != NULL)
			tmp->next->prev = tmp->prev;
		free(tmp);
	}
	return tmp;
}

int evm_msgid_prepare_cb_set(evm_evids_list_struct *msgid_ptr, int (*ev_prepare)(void *ev_ptr))
{
	int rv = 0;
	evm_log_info("(entry)\n");

	if (msgid_ptr == NULL)
		return -1;

	if (ev_prepare == NULL)
		return -1;

	if (rv == 0) {
		msgid_ptr->ev_prepare = ev_prepare;
	}
	return rv;
}

int evm_msgid_handle_cb_set(evm_evids_list_struct *msgid_ptr, int (*ev_handle)(void *ev_ptr))
{
	int rv = 0;
	evm_log_info("(entry)\n");

	if (msgid_ptr == NULL)
		return -1;

	if (ev_handle == NULL)
		return -1;

	if (rv == 0) {
		msgid_ptr->ev_handle = ev_handle;
	}
	return rv;
}

int evm_msgid_finalize_cb_set(evm_evids_list_struct *msgid_ptr, int (*ev_finalize)(void *ev_ptr))
{
	int rv = 0;
	evm_log_info("(entry)\n");

	if (msgid_ptr == NULL)
		return -1;

	if (ev_finalize == NULL)
		return -1;

	if (rv == 0) {
		msgid_ptr->ev_finalize = ev_finalize;
	}
	return rv;
}

