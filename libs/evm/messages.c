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

#include "evm/libevm.h"

#include "evm.h"
#include "messages.h"

#include "userlog/log_module.h"
EVMLOG_MODULE_INIT(EVM_MSGS, 1)

static int msg_enqueue(evm_consumer_struct *consumer, evm_message_struct *msg);
static evm_message_struct * msg_dequeue(evm_consumer_struct *consumer);

msgs_queue_struct * messages_consumer_queue_init(evm_consumer_struct *consumer)
{
	msgs_queue_struct *msgs_queue = NULL;
	evm_log_info("(entry)\n");

	if (consumer == NULL) {
		evm_log_error("Event machine consumer object undefined!\n");
		return NULL;
	}

	/* Setup internal message queue. */
	if ((msgs_queue = calloc(1, sizeof(msgs_queue_struct))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): internal message queue\n");
		return NULL;
	}
	consumer->msgs_queue = msgs_queue;
	pthread_mutex_init(&consumer->msgs_queue->access_mutex, NULL);
	pthread_mutex_unlock(&consumer->msgs_queue->access_mutex);

	return msgs_queue;
}

static int msg_enqueue(evm_consumer_struct *consumer, evm_message_struct *msg)
{
	int rv = 0;
	msgs_queue_struct *msgs_queue;
	msg_hanger_struct *msg_hanger;
	sem_t *bsem;
	pthread_mutex_t *amtx;
	evm_log_info("(entry)\n");

	if (consumer != NULL) {
		msgs_queue = consumer->msgs_queue;
		bsem = &consumer->blocking_sem;
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

static evm_message_struct * msg_dequeue(evm_consumer_struct *consumer)
{
	evm_message_struct *msg;
	msgs_queue_struct *msgs_queue;
	msg_hanger_struct *msg_hanger;
	sem_t *bsem;
	pthread_mutex_t *amtx;
	evm_log_info("(entry)\n");

	if (consumer != NULL) {
		msgs_queue = (msgs_queue_struct *)consumer->msgs_queue;
		bsem = &consumer->blocking_sem;
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
	free(msg_hanger);
	msg_hanger = NULL;
	pthread_mutex_unlock(amtx);
	return msg;
}

evm_message_struct * messages_check(evm_consumer_struct *consumer)
{
	evm_log_info("(entry)\n");

	if (consumer == NULL) {
		evm_log_error("Event machine consumer object undefined!\n");
		abort();
	}

	/* Poll the internal message queue first (THE ONLY POTENTIALLY BLOCKING POINT). */
	return msg_dequeue(consumer);
}

/*
 * Public API functions:
 * - evm_message_pass()
 * - evm_message_post()
 */
int evm_message_pass(evmConsumerStruct *consumer, evmMessageStruct *msg)
{
	evm_log_info("(entry) consumer=%p, msg=%p\n", consumer, msg);

	if ((consumer != NULL) && (msg != NULL)) {
		pthread_mutex_lock(&msg->amtx);
		if (msg_enqueue(consumer, msg) != 0) {
			evm_log_error("Message enqueuing failed!\n");
			pthread_mutex_unlock(&msg->amtx);
			return -1;
		}
		msg->consumers++;
		pthread_mutex_unlock(&msg->amtx);
		return 0;
	}
	return -1;
}

int evm_message_post(evmTopicStruct *topic, evmMessageStruct *msg)
{
	int rv = 0;
	evmlist_el_struct *tmp;
	evmConsumerStruct *consumer;
	evm_log_info("(entry) topic=%p, msg=%p\n", topic, msg);

	if ((topic != NULL) && (msg != NULL)) {
		pthread_mutex_lock(&topic->consumers_list->access_mutex);
		pthread_mutex_lock(&msg->amtx);
		for (
			tmp = topic->consumers_list->first;
			tmp != NULL;
			tmp = tmp->next
		) {
			consumer = (evm_consumer_struct *)tmp->el;
			if (msg_enqueue(consumer, msg) != 0) {
				evm_log_error("Message enqueuing failed!\n");
				rv++;
			}
			msg->consumers++;
		}
		pthread_mutex_unlock(&msg->amtx);
		pthread_mutex_unlock(&topic->consumers_list->access_mutex);
	}
	return rv;
}

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
			tmp = evm_search_evmlist(evm->msgtypes_list, id);
			if ((tmp != NULL) && (tmp->id == id)) {
				/* required id already exists - return existing element */
				msgtype = (evm_msgtype_struct *)tmp->el;
			} else {
				/* create new evmlist element with id */
				if ((new = evm_new_evmlist_el(id)) != NULL) {
					/* create new msgtype */
					if ((msgtype = (evm_msgtype_struct *)calloc(1, sizeof(evm_msgtype_struct))) == NULL) {
						errno = ENOMEM;
						evm_log_system_error("calloc(): msgtype\n");
						free(new);
						new = NULL;
					}
					if (msgtype != NULL) {
						msgtype->evm = evm;
						msgtype->id = id;
						msgtype->msgtype_parse = NULL;
						if ((msgtype->msgids_list = calloc(1, sizeof(evmlist_head_struct))) == NULL) {
							errno = ENOMEM;
							evm_log_system_error("calloc(): msgtype->msgids_list\n");
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
			tmp = evm_search_evmlist(evm->msgtypes_list, id);
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
			tmp = evm_search_evmlist(evm->msgtypes_list, id);
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
			tmp = evm_search_evmlist(msgtype->msgids_list, id);
			if ((tmp != NULL) && (tmp->id == id)) {
				/* required id already exists - return existing element */
				msgid = (evm_msgid_struct *)tmp->el;
			} else {
				/* create new evmlist element with id */
				if ((new = evm_new_evmlist_el(id)) != NULL) {
					/* create new msgid */
					if ((msgid = (evm_msgid_struct *)calloc(1, sizeof(evm_msgid_struct))) == NULL) {
						errno = ENOMEM;
						evm_log_system_error("calloc(): msgid\n");
						free(new);
						new = NULL;
					}
					if (msgid != NULL) {
						msgid->evm = msgtype->evm;
						msgid->msgtype = msgtype;
						msgid->id = id;
						msgid->msg_handle = NULL;
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
			tmp = evm_search_evmlist(msgtype->msgids_list, id);
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
			tmp = evm_search_evmlist(msgtype->msgids_list, id);
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
 * - evm_msgid_cb_handle_set()
 */
int evm_msgid_cb_handle_set(evmMsgidStruct *msgid, int (*msg_handle)(evmConsumerStruct *consumer, evmMessageStruct *msg))
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

/*
 * Public API functions:
 * - evm_message_new()
 * - evm_message_delete()
 * - evm_message_alloc_add()
 * - evm_message_persistent_set()
 * - evm_message_ctx_set()
 * - evm_message_ctx_get()
 * - evm_message_data_get()
 * - evm_message_data_takeover()
 * - evm_message_lock()
 * - evm_message_unlock()
 */
evmMessageStruct * evm_message_new(evmMsgtypeStruct *msgtype, evmMsgidStruct *msgid, size_t size)
{
	evmMessageStruct *msg = NULL;
	evm_log_info("(entry)\n");

	if ((msg = (evmMessageStruct *)calloc(1, sizeof(evmMessageStruct))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): msg\n");
		return NULL;
	}
	if ((msg->allocs_list = calloc(1, sizeof(evmlist_head_struct))) == NULL) {
		free(msg);
		msg = NULL;
		return NULL;
	}
	pthread_mutex_init(&msg->allocs_list->access_mutex, NULL);
	pthread_mutex_unlock(&msg->allocs_list->access_mutex);
	msg->msgtype = msgtype;
	msg->msgid = msgid;
	pthread_mutex_init(&msg->amtx, NULL);
	pthread_mutex_unlock(&msg->amtx);
	if (size > 0)
		if ((msg->data = malloc(size)) == NULL) {
			errno = ENOMEM;
			evm_log_system_error("malloc(): data\n");
			free(msg);
			msg = NULL;
		}

	return msg;
}

void evm_message_delete(evmMessageStruct *msg)
{
	evmlist_el_struct *tmp;
	evm_log_info("(entry) msg=%p\n", msg);

	if (msg != NULL) {
		pthread_mutex_lock(&msg->amtx);
		if (msg->consumers > 0)
			msg->consumers--;
		if (msg->consumers != 0) {
			pthread_mutex_unlock(&msg->amtx);
		} else {
			if (msg->data != NULL) {
				free(msg->data);
				msg->data = NULL;
			}
			if (msg->allocs_list != NULL) {
				pthread_mutex_lock(&msg->allocs_list->access_mutex);
				tmp = msg->allocs_list->first;
				while (tmp != NULL) {
					if (tmp->el != NULL) {
						free(tmp->el);
						tmp->el = NULL;
					}
					if (tmp->next != NULL) {
						tmp = tmp->next;
						free(tmp->prev);
						tmp->prev = NULL;
					} else {
						free(tmp);
						tmp = NULL;
					}
				}
				pthread_mutex_unlock(&msg->allocs_list->access_mutex);
				free(msg->allocs_list);
				msg->allocs_list = NULL;
			}
			free(msg);
		}
	}
}

int evm_message_alloc_add(evmMessageStruct *msg, void *alloc)
{
	evmlist_el_struct *new, *tmp;
	evm_log_info("(entry)\n");

	if ((msg == NULL) || (alloc == NULL))
		return -1;

	if (msg->allocs_list != NULL) {
		/* create new evmlist element with dummy id (0) */
		if ((new = evm_new_evmlist_el(0)) != NULL) {
			pthread_mutex_lock(&msg->allocs_list->access_mutex);
			tmp = msg->allocs_list->first;
			if (tmp != NULL)
				tmp->prev = new;
			msg->allocs_list->first = new;
			new->prev = msg->allocs_list->first;
			new->next = tmp;
			new->el = alloc;
			pthread_mutex_unlock(&msg->allocs_list->access_mutex);
		} else
			return -1;
	}

	return 0;
}

int evm_message_persistent_set(evmMessageStruct *msg)
{
	evm_log_info("(entry)\n");

	if (msg == NULL)
		return -1;

	msg->consumers = 0xffff;
	return 0;
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

void * evm_message_data_get(evmMessageStruct *msg)
{
	evm_log_info("(entry)\n");

	if (msg == NULL)
		return NULL;

	return msg->data;
}

void * evm_message_data_takeover(evmMessageStruct *msg)
{
	void *ptr;
	evm_log_info("(entry)\n");

	if (msg == NULL)
		return NULL;

	ptr = msg->data;
	msg->data = NULL;

	return ptr;
}

int evm_message_lock(evmMessageStruct *msg)
{
	evm_log_info("(entry)\n");

	if (msg == NULL)
		return -1;

	return pthread_mutex_lock(&msg->amtx);
}

int evm_message_unlock(evmMessageStruct *msg)
{
	evm_log_info("(entry)\n");

	if (msg == NULL)
		return -1;

	return pthread_mutex_unlock(&msg->amtx);
}

