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

#if 0 /*samo - orig*/
static sigset_t msgs_sigmask; /*actually a constant initialized via sigemptymask()*/
/* Thread-specific data related. */
static int msg_thr_keys_created = EVM_FALSE;
static pthread_key_t msg_thr_signum_key; /*initialized only once - not for each call to evm_init() within a process*/

static void msgs_sighandler(int signum, siginfo_t *siginfo, void *context);
static int msgs_sighandler_install(int signum);
static int messages_receive(evm_fd_struct *evs_fd_ptr);
static int messages_parse(evm_fd_struct *evs_fd_ptr);
static int message_queue_evmfd_read(int efd, evm_message_struct *message);
#endif
static int msg_enqueue(evm_consumer_struct *consumer_ptr, evm_message_struct *msg);
static evm_message_struct * msg_dequeue(evm_consumer_struct *consumer_ptr);

#if 0 /*samo - orig*/
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
#endif

msgs_queue_struct * messages_queue_init(evm_consumer_struct *consumer_ptr)
{
#if 0 /*samo - orig*/
	void *ptr;
	sigset_t sigmask;
	evm_sigpost_struct *evm_sigpost = NULL;
#endif
	msgs_queue_struct *msgs_queue_ptr = NULL;
	evm_log_info("(entry)\n");

	if (consumer_ptr == NULL) {
		evm_log_error("Event machine consumer object undefined!\n");
		return NULL;
	}

#if 0 /*samo - orig*/
	if (consumer_ptr->evm != NULL)
		evm_sigpost = consumer_ptr->evm->evm_sigpost;
	if (evm_sigpost == NULL)
		evm_log_debug("Signal post-processing handler undefined!\n");

	/* Prepare thread-specific integer for thread related signal post-handling. */
	if (!msg_thr_keys_created) {
		if ((errno = pthread_key_create(&msg_thr_signum_key, NULL)) != 0) {
			evm_log_system_error("pthread_key_create()\n");
			return NULL;
		}
		msg_thr_keys_created = EVM_TRUE;
	}
	if ((ptr = calloc(1, sizeof(int))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): thread-specific data\n");
		return NULL;
	}
	if ((errno = pthread_setspecific(msg_thr_signum_key, ptr)) != 0) {
		evm_log_system_error("pthread_setspecific()\n");
		free(ptr);
		ptr = NULL;
		return NULL;
	}

	/* Prepare module-local empty signal mask, used in epoll_pwait() to allow catching all signals there!*/
	sigemptyset(&msgs_sigmask);

	/* Unblock all signals, except HUP and CHLD, which are allowed to get caught only in epoll_pwait()! */
	evm_log_debug("Unblocking all signals, except SIGHUP and SIGCHLD\n");
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGHUP);
	sigaddset(&sigmask, SIGCHLD);
	if (pthread_sigmask(SIG_SETMASK, &sigmask, NULL) < 0) {
		evm_log_system_error("sigprocmask() SIG_SETMASK\n");
		free(ptr);
		ptr = NULL;
		return NULL;
	}

	/* Install signal handler for SIGHUP and SIGCHLD. */
	evm_log_debug("Establishing handler for signal SIGHUP\n");
	if (msgs_sighandler_install(SIGHUP) < 0) {
		evm_log_system_error("Failed to install SIGHUP signal handler!\n");
		free(ptr);
		ptr = NULL;
		return NULL;
	}
	evm_log_debug("Establishing handler for signal SIGCHLD\n");
	if (msgs_sighandler_install(SIGCHLD) < 0) {
		evm_log_error("Failed to install SIGCHLD signal handler!\n");
		free(ptr);
		ptr = NULL;
		return NULL;
	}
#endif

	/* Setup internal message queue. */
	if ((msgs_queue_ptr = calloc(1, sizeof(msgs_queue_struct))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): internal message queue\n");
#if 0 /*samo - orig*/
		free(ptr);
		ptr = NULL;
#endif
		return NULL;
	}
	consumer_ptr->msgs_queue = msgs_queue_ptr;
	pthread_mutex_init(&consumer_ptr->msgs_queue->access_mutex, NULL);
	pthread_mutex_unlock(&consumer_ptr->msgs_queue->access_mutex);

	return msgs_queue_ptr;
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

static int msg_enqueue(evm_consumer_struct *consumer_ptr, evm_message_struct *msg)
{
	int rv = 0;
	msgs_queue_struct *msgs_queue;
	msg_hanger_struct *msg_hanger;
	sem_t *bsem;
	pthread_mutex_t *amtx;
	evm_log_info("(entry)\n");

	if (consumer_ptr != NULL) {
		msgs_queue = consumer_ptr->msgs_queue;
		bsem = &consumer_ptr->blocking_sem;
		if (msgs_queue != NULL)
			amtx = &msgs_queue->access_mutex;
		else
			rv = -1;
	} else
		rv = -1;

	if (rv == 0) {
		pthread_mutex_lock(amtx);
		if ((msg_hanger = (msg_hanger_struct *)calloc(1, sizeof(msg_hanger_struct))) == NULL) {
			errno = ENOMEM;
			evm_log_system_error("calloc(): message hanger\n");
			pthread_mutex_unlock(amtx);
			return -1;
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
	}
	return rv;
}

static evm_message_struct * msg_dequeue(evm_consumer_struct *consumer_ptr)
{
	evm_message_struct *msg;
	msgs_queue_struct *msgs_queue;
	msg_hanger_struct *msg_hanger;
	sem_t *bsem;
	pthread_mutex_t *amtx;
	evm_log_info("(entry)\n");

	if (consumer_ptr != NULL) {
		msgs_queue = (msgs_queue_struct *)consumer_ptr->msgs_queue;
		bsem = &consumer_ptr->blocking_sem;
		if (msgs_queue != NULL)
			amtx = &msgs_queue->access_mutex;
		else
			return NULL;
	} else
		return NULL;


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
	msg->consumer = consumer_ptr; /*set dequeueing consumer*/
	free(msg_hanger);
	msg_hanger = NULL;
	pthread_mutex_unlock(amtx);
	return msg;
}

evm_message_struct * messages_check(evm_consumer_struct *consumer_ptr)
{
#if 0 /*samo - orig*/
	int status = 0;
	evm_fd_struct *evs_fd_ptr = NULL;
#endif
	msgs_queue_struct *msgs_queue;
	evm_log_info("(entry)\n");

	if (consumer_ptr == NULL) {
		evm_log_error("Event machine consumer object undefined!\n");
		abort();
	}

	msgs_queue = consumer_ptr->msgs_queue;
	evm_log_info("queue->first=%p\n", msgs_queue->first_hanger);

	/* Poll the internal message queue first (THE ONLY POTENTIALLY BLOCKING POINT). */
	return msg_dequeue(consumer_ptr);

#if 0 /*samo - orig*/
	/* Receive any data. */
	if ((status = messages_receive(evs_fd_ptr)) < 0) {
		evm_log_debug("messages_receive() returned %d\n", status);
		return NULL;
	}

	if (evs_fd_ptr == ((msgs_queue_struct *)consumer_ptr->msgs_queue)->evmfd) {
		evm_log_debug("Internal queue receive triggered!\n");
#if 0 /*test*/
		if (msgs_queue->first_hanger != NULL) {
			return msg_dequeue(consumer_ptr);
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

/*
 * Public API functions:
 * - evm_message_pass()
 * - evm_message_concatenate()
 */
int evm_message_pass(evmConsumerStruct *consumer, evmMessageStruct *msg)
{
	evm_log_info("(entry) consumer=%p, msg=%p\n", consumer, msg);

	if (consumer != NULL) {
		if (msg_enqueue(consumer, msg) != 0) {
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

/*
 * Public API functions:
 * - evm_msgtype_add()
 * - evm_msgtype_get()
 * - evm_msgtype_del()
 */
evmMsgtypeStruct * evm_msgtype_add(evmStruct *evm, int id)
{
	evmMsgtypeStruct *msgtype = NULL;
	evmlist_el_struct *tmp, *new;
	evm_log_info("(entry)\n");

	if (evm != NULL) {
		if (evm->msgtypes_list != NULL) {
			pthread_mutex_lock(&evm->msgtypes_list->access_mutex);
			tmp = evm_walk_evmlist(evm->msgtypes_list, id);
			if ((tmp != NULL) && (tmp->id == id)) {
				/* required id already exists - return existing element */
				msgtype = (evm_msgtype_struct *)tmp->el;
			} else {
				/* create new evmlist element with id */
				if ((new = evm_new_evmlist_el(id)) != NULL) {
					/* create new msgtype */
					if ((msgtype = (evm_msgtype_struct *)calloc(1, sizeof(evm_msgtype_struct))) == NULL) {
						errno = ENOMEM;
						evm_log_system_error("calloc(): (evm_msgtype_struct)msgtype\n");
						free(new);
						new = NULL;
					}
					if (msgtype != NULL) {
						msgtype->evm = evm;
						msgtype->id = id;
						msgtype->msgtype_parse = NULL;
						if ((msgtype->msgids_list = calloc(1, sizeof(evmlist_head_struct))) == NULL) {
							errno = ENOMEM;
							evm_log_system_error("calloc(): (evmlist_head_struct)msgtype->msgids_list\n");
							free(msgtype);
							msgtype = NULL;
							free(new);
							new = NULL;
						} else {
							pthread_mutex_init(&evm->tmrids_list->access_mutex, NULL);
							pthread_mutex_unlock(&evm->tmrids_list->access_mutex);
						}
					}
				}
				if (new != NULL) {
					new->id = id;
					new->el = (void *)msgtype;
					new->prev = tmp;
					new->next = NULL;
					if (tmp != NULL)
						tmp->next = new;
					else
						evm->msgtypes_list->first = new;
				}
			}
			pthread_mutex_unlock(&evm->msgtypes_list->access_mutex);
		}	
	}
	return msgtype;
}

evmMsgtypeStruct * evm_msgtype_get(evmStruct *evm, int id)
{
	evmMsgtypeStruct *msgtype = NULL;
	evmlist_el_struct *tmp;

	if (evm != NULL) {
		if (evm->msgtypes_list != NULL) {
			pthread_mutex_lock(&evm->msgtypes_list->access_mutex);
			tmp = evm_walk_evmlist(evm->msgtypes_list, id);
			if ((tmp != NULL) && (tmp->id == id)) {
				/* required id already exists - return existing element */
				msgtype = (evm_msgtype_struct *)tmp->el;
			}
			pthread_mutex_unlock(&evm->msgtypes_list->access_mutex);
		}	
	}
	return msgtype;
}

evmMsgtypeStruct * evm_msgtype_del(evmStruct *evm, int id)
{
	evmMsgtypeStruct *msgtype = NULL;
	evmlist_el_struct *tmp;

	if (evm != NULL) {
		if (evm->msgtypes_list != NULL) {
			pthread_mutex_lock(&evm->msgtypes_list->access_mutex);
			tmp = evm_walk_evmlist(evm->msgtypes_list, id);
			if ((tmp != NULL) && (tmp->id == id)) {
				/* required id already exists - return existing element */
				msgtype = (evm_msgtype_struct *)tmp->el;
				if (msgtype != NULL) {
					free(msgtype);
				}
				tmp->prev->next = tmp->next;
				if (tmp->next != NULL)
					tmp->next->prev = tmp->prev;
				free(tmp);
				tmp = NULL;
			}
			pthread_mutex_unlock(&evm->msgtypes_list->access_mutex);
		}	
	}
	return msgtype;
}

/*
 * Public API function:
 * - evm_msgtype_cb_parse_set()
 */
int evm_msgtype_cb_parse_set(evmMsgtypeStruct *msgtype, int (*msgtype_parse)(void *ptr))
{
	int rv = 0;
	evm_log_info("(entry)\n");

	if (msgtype == NULL)
		return -1;

	if (msgtype_parse == NULL)
		return -1;

	if (rv == 0) {
		msgtype->msgtype_parse = msgtype_parse;
	}
	return rv;
}

/*
 * Public API functions:
 * - evm_msgid_add()
 * - evm_msgid_get()
 * - evm_msgid_del()
 */
evmMsgidStruct * evm_msgid_add(evmMsgtypeStruct *msgtype, int id)
{
	evmMsgidStruct *msgid = NULL;
	evmlist_el_struct *tmp, *new;
	evm_log_info("(entry)\n");

	if (msgtype != NULL) {
		if (msgtype->msgids_list != NULL) {
			pthread_mutex_lock(&msgtype->msgids_list->access_mutex);
			tmp = evm_walk_evmlist(msgtype->msgids_list, id);
			if ((tmp != NULL) && (tmp->id == id)) {
				/* required id already exists - return existing element */
				msgid = (evm_msgid_struct *)tmp->el;
			} else {
				/* create new evmlist element with id */
				if ((new = evm_new_evmlist_el(id)) != NULL) {
					/* create new msgid */
					if ((msgid = (evm_msgid_struct *)calloc(1, sizeof(evm_msgid_struct))) == NULL) {
						errno = ENOMEM;
						evm_log_system_error("calloc(): (evm_msgid_struct)msgid\n");
						free(new);
						new = NULL;
					}
					if (msgid != NULL) {
						msgid->evm = msgtype->evm;
						msgid->msgtype = msgtype;
						msgid->id = id;
						msgid->msg_prepare = NULL;
						msgid->msg_handle = NULL;
						msgid->msg_finalize = NULL;
					}
				}
				if (new != NULL) {
					new->id = id;
					new->el = (void *)msgid;
					new->prev = tmp;
					new->next = NULL;
					if (tmp != NULL)
						tmp->next = new;
					else
						msgtype->msgids_list->first = new;
				}
			}
			pthread_mutex_unlock(&msgtype->msgids_list->access_mutex);
		}	
	}
	return msgid;
}

evmMsgidStruct * evm_msgid_get(evmMsgtypeStruct *msgtype, int id)
{
	evmMsgidStruct *msgid = NULL;
	evmlist_el_struct *tmp;

	if (msgtype != NULL) {
		if (msgtype->msgids_list != NULL) {
			pthread_mutex_lock(&msgtype->msgids_list->access_mutex);
			tmp = evm_walk_evmlist(msgtype->msgids_list, id);
			if ((tmp != NULL) && (tmp->id == id)) {
				/* required id already exists - return existing element */
				msgid = (evm_msgid_struct *)tmp->el;
			}
			pthread_mutex_unlock(&msgtype->msgids_list->access_mutex);
		}	
	}
	return msgid;
}

evmMsgidStruct * evm_msgid_del(evmMsgtypeStruct *msgtype, int id)
{
	evmMsgidStruct *msgid = NULL;
	evmlist_el_struct *tmp;

	if (msgtype != NULL) {
		if (msgtype->msgids_list != NULL) {
			pthread_mutex_lock(&msgtype->msgids_list->access_mutex);
			tmp = evm_walk_evmlist(msgtype->msgids_list, id);
			if ((tmp != NULL) && (tmp->id == id)) {
				/* required id already exists - return existing element */
				msgid = (evm_msgid_struct *)tmp->el;
				if (msgid != NULL) {
					free(msgid);
				}
				tmp->prev->next = tmp->next;
				if (tmp->next != NULL)
					tmp->next->prev = tmp->prev;
				free(tmp);
				tmp = NULL;
			}
			pthread_mutex_unlock(&msgtype->msgids_list->access_mutex);
		}	
	}
	return (evmMsgidStruct *)msgid;
}

/*
 * Public API functions:
 * - evm_msgid_cb_prepare_set()
 * - evm_msgid_cb_handle_set()
 * - evm_msgid_cb_finalize_set()
 */
int evm_msgid_cb_prepare_set(evmMsgidStruct *msgid, int (*msg_prepare)(void *ptr))
{
	int rv = 0;
	evm_log_info("(entry)\n");

	if (msgid == NULL)
		return -1;

	if (msg_prepare == NULL)
		return -1;

	if (rv == 0) {
		msgid->msg_prepare = msg_prepare;
	}
	return rv;
}

int evm_msgid_cb_handle_set(evmMsgidStruct *msgid, int (*msg_handle)(void *ptr))
{
	int rv = 0;
	evm_log_info("(entry)\n");

	if (msgid == NULL)
		return -1;

	if (msg_handle == NULL)
		return -1;

	if (rv == 0) {
		msgid->msg_handle = msg_handle;
	}
	return rv;
}

int evm_msgid_cb_finalize_set(evmMsgidStruct *msgid, int (*msg_finalize)(void *ptr))
{
	int rv = 0;
	evm_log_info("(entry)\n");

	if (msgid == NULL)
		return -1;

	if (msg_finalize == NULL)
		return -1;

	if (rv == 0) {
		msgid->msg_finalize = msg_finalize;
	}
	return rv;
}

/*
 * Public API functions:
 * - evm_message_new()
 * - evm_message_delete()
 * - evm_message_consumer_get()
 * - evm_message_ctx_set()
 * - evm_message_ctx_get()
 * - evm_message_iovec_set()
 * - evm_message_iovec_get()
 */
evmMessageStruct * evm_message_new(evmMsgtypeStruct *msgtype, evmMsgidStruct *msgid)
{
	evmMessageStruct *msg = NULL;
	evm_log_info("(entry)\n");

	if ((msg = (evmMessageStruct *)calloc(1, sizeof(evmMessageStruct))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): (evmMessageStruct)msg\n");
		return NULL;
	}
	msg->msgtype = msgtype;
	msg->msgid = msgid;
	return msg;
}

void evm_message_delete(evmMessageStruct *msg)
{
	evm_log_info("(entry)\n");

	if (msg != NULL) {
		free(msg);
		msg = NULL;
	}
}

evmConsumerStruct * evm_message_consumer_get(evmMessageStruct *msg)
{
	evm_log_info("(entry)\n");

	if (msg == NULL)
		return NULL;

	return (msg->consumer);
}

int evm_message_ctx_set(evmMessageStruct *msg, void *ctx)
{
	evm_log_info("(entry)\n");

	if (msg == NULL)
		return -1;

	if (ctx == NULL)
		return -1;

	msg->ctx = ctx;
	return 0;
}

void * evm_message_ctx_get(evmMessageStruct *msg)
{
	evm_log_info("(entry)\n");

	if (msg == NULL)
		return NULL;

	return (msg->ctx);
}

int evm_message_iovec_set(evmMessageStruct *msg, struct iovec *iov_buff)
{
	evm_log_info("(entry)\n");

	if (msg == NULL)
		return -1;

	if (iov_buff == NULL)
		return -1;

	msg->iov_buff = iov_buff;
	return 0;
}

struct iovec * evm_message_iovec_get(evmMessageStruct *msg)
{
	evm_log_info("(entry)\n");

	if (msg == NULL)
		return NULL;

	return (msg->iov_buff);
}

