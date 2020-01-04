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
#include <pthread.h>

#include "evm/libevm.h"

#include "evm.h"
#include "messages.h"
#include "timers.h"

#include "userlog/log_module.h"
EVMLOG_MODULE_INIT(EVM_CORE, 1)

/*current project version*/
unsigned int evmVerMajor = EVM_VERSION_MAJOR;
unsigned int evmVerMinor = EVM_VERSION_MINOR;
unsigned int evmVerPatch = EVM_VERSION_PATCH;

static int prepare_msg(evm_msgid_struct *msgid, void *msg);
static int handle_msg(evm_msgid_struct *msgid, void *msg);
static int finalize_msg(evm_msgid_struct *msgid, void *msg);

static int handle_tmr(evm_tmrid_struct *tmrid, void *tmr);
static int finalize_tmr(evm_tmrid_struct *tmrid, void *tmr);

static int handle_timer(evm_timer_struct *expdTmr);
static int handle_message(evm_message_struct *recvdMsg);

/*
 * Public API function:
 * - evm_init()
 *
 * Main event machine initialization
 */
evmStruct * evm_init(void)
{
	evmStruct *evm = NULL;

	if ((evm = calloc(1, sizeof(evmStruct))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): evm\n");
	}
	if (evm != NULL) {
		if ((evm->msgtypes_list = calloc(1, sizeof(evmlist_head_struct))) == NULL) {
			errno = ENOMEM;
			evm_log_system_error("calloc(): evm->msgtypes_list\n");
			free(evm);
			evm = NULL;
		} else {
			pthread_mutex_init(&evm->msgtypes_list->access_mutex, NULL);
			pthread_mutex_unlock(&evm->msgtypes_list->access_mutex);
		}
	}
	if (evm != NULL) {
		if ((evm->tmrids_list = calloc(1, sizeof(evmlist_head_struct))) == NULL) {
			errno = ENOMEM;
			evm_log_system_error("calloc(): evm->tmrids_list\n");
			free(evm->msgtypes_list);
			evm->msgtypes_list = NULL;
			free(evm);
			evm = NULL;
		} else {
			pthread_mutex_init(&evm->tmrids_list->access_mutex, NULL);
			pthread_mutex_unlock(&evm->tmrids_list->access_mutex);
		}
	}
	if (evm != NULL) {
		if ((evm->consumers_list = calloc(1, sizeof(evmlist_head_struct))) == NULL) {
			errno = ENOMEM;
			evm_log_system_error("calloc(): evm->consumers_list\n");
			free(evm->tmrids_list);
			evm->tmrids_list = NULL;
			free(evm->msgtypes_list);
			evm->msgtypes_list = NULL;
			free(evm);
			evm = NULL;
		} else {
			pthread_mutex_init(&evm->consumers_list->access_mutex, NULL);
			pthread_mutex_unlock(&evm->consumers_list->access_mutex);
		}
	}
	if (evm != NULL) {
		if ((evm->topics_list = calloc(1, sizeof(evmlist_head_struct))) == NULL) {
			errno = ENOMEM;
			evm_log_system_error("calloc(): evm->topics_list\n");
			free(evm->consumers_list);
			evm->consumers_list = NULL;
			free(evm->tmrids_list);
			evm->tmrids_list = NULL;
			free(evm->msgtypes_list);
			evm->msgtypes_list = NULL;
			free(evm);
			evm = NULL;
		} else {
			pthread_mutex_init(&evm->topics_list->access_mutex, NULL);
			pthread_mutex_unlock(&evm->topics_list->access_mutex);
		}
	}
	return evm;
}

/*
 * Internally global "evmlist" helper function
 */
evmlist_el_struct * evm_search_evmlist(evmlist_head_struct *head, int id)
{
	evmlist_el_struct *tmp = NULL;
	evm_log_info("(entry) head=%p\n", head);

	if (head != NULL) {
		tmp = head->first;
		while (tmp != NULL) {
			if (tmp->next == NULL)
				break;
			if (tmp->id == id)
				break;
			tmp = tmp->next;
		}
	}
	return tmp;
}

/*
 * Internally global "evmlist" helper function
 */
evmlist_el_struct * evm_new_evmlist_el(int id)
{
	evmlist_el_struct *new = NULL;

	if ((new = calloc(1, sizeof(evmlist_el_struct))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): new\n");
	} else {
		new->id = id;
		new->next = NULL;
	}

	return new;
}

/*
 * Public API functions:
 * - evm_consumer_add()
 * - evm_consumer_get()
 * - evm_consumer_del()
 */
evmConsumerStruct * evm_consumer_add(evmStruct *evm, int id)
{
	evmConsumerStruct *consumer = NULL;
	evmlist_el_struct *tmp, *new;
	evm_log_info("(entry) evm=%p, id=%d\n", evm, id);

	if (evm != NULL) {
		if (evm->consumers_list != NULL) {
			pthread_mutex_lock(&evm->consumers_list->access_mutex);
			tmp = evm_search_evmlist(evm->consumers_list, id);
			if ((tmp != NULL) && (tmp->id == id)) {
				/* required id already exists - return existing element */
				consumer = (evm_consumer_struct *)tmp->el;
			} else {
				/* create new evmlist element with id */
				if ((new = evm_new_evmlist_el(id)) != NULL) {
					/* create new consumer */
					if ((consumer = (evm_consumer_struct *)calloc(1, sizeof(evm_consumer_struct))) == NULL) {
						errno = ENOMEM;
						evm_log_system_error("calloc(): consumer\n");
						free(new);
						new = NULL;
					}
					if (consumer != NULL) {
						consumer->evm = evm;
						consumer->id = id;
						if (sem_init(&consumer->blocking_sem, 0, 0) != 0) {
							free(consumer);
							consumer = NULL;
							free(new);
							new = NULL;
						}
					}
					/*prepare per consumer msgs and tmrs queues here*/
					if (consumer != NULL) {
						/* Initialize timers infrastructure... */
						if (timers_queue_init(consumer) == NULL) {
							free(consumer);
							consumer = NULL;
							free(new);
							new = NULL;
						}
					}
					if (consumer != NULL) {
						/* Initialize EVM messages infrastructure... */
						if (messages_consumer_queue_init(consumer) == NULL) {
							free(consumer);
							consumer = NULL;
							free(new);
							new = NULL;
						}
					}
				}
				if (new != NULL) {
					new->el = (void *)consumer;
					new->prev = tmp;
					new->next = NULL;
					if (tmp != NULL)
						tmp->next = new;
					else
						evm->consumers_list->first = new;
				}
			}
			pthread_mutex_unlock(&evm->consumers_list->access_mutex);
		}	
	}
	return consumer;
}

evmConsumerStruct * evm_consumer_get(evmStruct *evm, int id)
{
	evmConsumerStruct *consumer = NULL;
	evmlist_el_struct *tmp;
	evm_log_info("(entry)\n");

	if (evm != NULL) {
		if (evm->consumers_list != NULL) {
			pthread_mutex_lock(&evm->consumers_list->access_mutex);
			tmp = evm_search_evmlist(evm->consumers_list, id);
			if ((tmp != NULL) && (tmp->id == id)) {
				/* required id already exists - return existing element */
				consumer = (evm_consumer_struct *)tmp->el;
			}
			pthread_mutex_unlock(&evm->consumers_list->access_mutex);
		}	
	}
	return consumer;
}

evmConsumerStruct * evm_consumer_del(evmStruct *evm, int id)
{
	evmConsumerStruct *consumer = NULL;
	evmlist_el_struct *tmp;
	evm_log_info("(entry)\n");

	if (evm != NULL) {
		if (evm->consumers_list != NULL) {
			pthread_mutex_lock(&evm->consumers_list->access_mutex);
			tmp = evm_search_evmlist(evm->consumers_list, id);
			if ((tmp != NULL) && (tmp->id == id)) {
				/* required id already exists - return existing element */
				consumer = (evm_consumer_struct *)tmp->el;
				if (consumer != NULL) {
					free(consumer);
				}
				tmp->prev->next = tmp->next;
				if (tmp->next != NULL)
					tmp->next->prev = tmp->prev;
				free(tmp);
				tmp = NULL;
			}
			pthread_mutex_unlock(&evm->consumers_list->access_mutex);
		}	
	}
	return consumer;
}

/*
 * Public API functions:
 * - evm_topic_add()
 * - evm_topic_get()
 * - evm_topic_del()
 */
evmTopicStruct * evm_topic_add(evmStruct *evm, int id)
{
	evmTopicStruct *topic = NULL;
	evmlist_el_struct *tmp, *new;
	evm_log_info("(entry)\n");

	if (evm != NULL) {
		if (evm->topics_list != NULL) {
			pthread_mutex_lock(&evm->topics_list->access_mutex);
			tmp = evm_search_evmlist(evm->topics_list, id);
			if ((tmp != NULL) && (tmp->id == id)) {
				/* required id already exists - return existing element */
				topic = (evm_topic_struct *)tmp->el;
			} else {
				/* create new evmlist element with id */
				if ((new = evm_new_evmlist_el(id)) != NULL) {
					/* create new topic */
					if ((topic = (evm_topic_struct *)calloc(1, sizeof(evm_topic_struct))) == NULL) {
						errno = ENOMEM;
						evm_log_system_error("calloc(): topic\n");
						free(new);
						new = NULL;
					}
					if (topic != NULL) {
						topic->evm = evm;
						topic->id = id;

						/* Initialize topic's message queue... */
						if (messages_topic_queue_init(topic) == NULL) {
							free(topic);
							topic = NULL;
							free(new);
							new = NULL;
						}
					}
				}
				if (new != NULL) {
					new->el = (void *)topic;
					new->prev = tmp;
					new->next = NULL;
					if (tmp != NULL)
						tmp->next = new;
					else
						evm->topics_list->first = new;
				}
			}
			pthread_mutex_unlock(&evm->topics_list->access_mutex);
		}	
	}
	return topic;
}

evmTopicStruct * evm_topic_get(evmStruct *evm, int id)
{
	evmTopicStruct *topic = NULL;
	evmlist_el_struct *tmp;
	evm_log_info("(entry)\n");

	if (evm != NULL) {
		if (evm->topics_list != NULL) {
			pthread_mutex_lock(&evm->topics_list->access_mutex);
			tmp = evm_search_evmlist(evm->topics_list, id);
			if ((tmp != NULL) && (tmp->id == id)) {
				/* required id already exists - return existing element */
				topic = (evm_topic_struct *)tmp->el;
			}
			pthread_mutex_unlock(&evm->topics_list->access_mutex);
		}	
	}
	return topic;
}

evmTopicStruct * evm_topic_del(evmStruct *evm, int id)
{
	evmTopicStruct *topic = NULL;
	evmlist_el_struct *tmp;
	evm_log_info("(entry)\n");

	if (evm != NULL) {
		if (evm->topics_list != NULL) {
			pthread_mutex_lock(&evm->topics_list->access_mutex);
			tmp = evm_search_evmlist(evm->topics_list, id);
			if ((tmp != NULL) && (tmp->id == id)) {
				/* required id already exists - return existing element */
				topic = (evm_topic_struct *)tmp->el;
				if (topic != NULL) {
					free(topic);
				}
				tmp->prev->next = tmp->next;
				if (tmp->next != NULL)
					tmp->next->prev = tmp->prev;
				free(tmp);
				tmp = NULL;
			}
			pthread_mutex_unlock(&evm->topics_list->access_mutex);
		}	
	}
	return topic;
}

/*
 * Public API functions:
 * - evm_priv_set()
 * - evm_priv_get()
 * - evm_sigpost_set()
 */
int evm_priv_set(evmStruct *evm, void *priv)
{
	int rv = 0;
	evm_log_info("(entry)\n");

	if (evm == NULL)
		return -1;

	if (priv == NULL)
		return -1;

	if (rv == 0) {
		evm->priv = priv;
	}
	return rv;
}

void * evm_priv_get(evmStruct *evm)
{
	evm_log_info("(entry)\n");

	if (evm == NULL)
		return NULL;

	return (evm->priv);
}

#if 0 /*samo - orig*/
int evm_sigpost_set(evmStruct *evm, evm_sigpost_struct *sigpost)
{
	int rv = 0;
	evm_log_info("(entry)\n");

	if (evm == NULL)
		return -1;

	if (sigpost == NULL)
		return -1;

	if (rv == 0) {
		evm->evm_sigpost = sigpost;
	}
	return rv;
}
#endif

/*
 * Public API functions:
 * - evm_run_once()
 * - evm_run_async()
 * - evm_run()
 *
 * Main event machine single pass-through
 */
int evm_run_once(evmConsumerStruct *consumer)
{
	int status = 0;
	evm_timer_struct *expdTmr;
	evm_message_struct *recvdMsg;
	evm_log_info("(entry)\n");

	if (consumer == NULL) {
		evm_log_error("Event machine consumer object undefined!\n");
		abort();
	}

	/* Loop exclusively over expired timers (non-blocking already)! */
	for (;;) {
		evm_log_info("(main loop entry)\n");
		/* Handle expired timer (NON-BLOCKING). */
		if ((expdTmr = timers_check(consumer)) != NULL) {
			if ((status = handle_timer(expdTmr)) < 0)
				evm_log_debug("handle_timer() returned %d\n", status);
		} else
			break;
	}

	/* Handle handle received message (WAIT - THE ONLY POTENTIALLY BLOCKING POINT). */
	if ((recvdMsg = messages_check(consumer)) != NULL) {
		if ((status = handle_message(recvdMsg)) < 0)
			evm_log_debug("handle_message() returned %d\n", status);
	}

	return 0;
}

/*
 * Main event machine single pass-through (asynchronous - nonblocking)
 */
int evm_run_async(evmConsumerStruct *consumer)
{
	sem_t *bsem;
	evm_log_info("(entry)\n");

	if (consumer == NULL) {
		evm_log_error("Event machine consumer object undefined!\n");
		abort();
	}

	bsem = &consumer->blocking_sem;
	evm_log_info("Post blocking semaphore (UNBLOCK - prevent blocking)\n");
	sem_post(bsem);
	return evm_run_once(consumer);
}

/*
 * Main event machine loop
 */
int evm_run(evmConsumerStruct *consumer)
{
	int status = 0;
	evm_timer_struct *expdTmr;
	evm_message_struct *recvdMsg;
	evm_log_info("(entry)\n");

	if (consumer == NULL) {
		evm_log_error("Event machine consumer object undefined!\n");
		abort();
	}

	/* Main protocol loop! */
	for (;;) {
		evm_log_info("(main loop entry)\n");
		/* Handle expired timer (NON-BLOCKING). */
		if ((expdTmr = timers_check(consumer)) != NULL) {
			if ((status = handle_timer(expdTmr)) < 0)
				evm_log_debug("handle_timer() returned %d\n", status);
			continue;
		}

		/* Handle handle received message (WAIT - THE ONLY BLOCKING POINT). */
		if ((recvdMsg = messages_check(consumer)) != NULL) {
			if ((status = handle_message(recvdMsg)) < 0)
				evm_log_debug("handle_message() returned %d\n", status);
		}
	}

	return 0;
}

/*
 * Public API functions:
 * - evm_consumer_priv_set()
 * - evm_consumer_priv_get()
 */
int evm_consumer_priv_set(evmConsumerStruct *consumer, void *priv)
{
	int rv = 0;
	evm_log_info("(entry)\n");

	if (consumer == NULL)
		return -1;

	if (priv == NULL)
		return -1;

	if (rv == 0) {
		consumer->priv = priv;
	}
	return rv;
}

void * evm_consumer_priv_get(evmConsumerStruct *consumer)
{
	evm_log_info("(entry)\n");

	if (consumer == NULL)
		return NULL;

	return (consumer->priv);
}

static int prepare_msg(evm_msgid_struct *msgid, void *msg)
{
	if (msgid != NULL) {
		if (msgid->msg_prepare != NULL)
			return msgid->msg_prepare(msg);
		else
			return 0;
	}
	return -1;
}

static int handle_msg(evm_msgid_struct *msgid, void *msg)
{
	if (msgid != NULL)
		if (msgid->msg_handle != NULL)
			return msgid->msg_handle(msg);
	return -1;
}

static int finalize_msg(evm_msgid_struct *msgid, void *msg)
{
	if (msgid != NULL)
		if (msgid->msg_finalize != NULL)
			return msgid->msg_finalize(msg);
	return -1;
}

static int handle_message(evm_message_struct *recvdMsg)
{
	int status0 = 0;
	int status1 = 0;
	int status2 = 0;

	status0 = prepare_msg(recvdMsg->msgid, recvdMsg);
	if (status0 < 0)
		evm_log_debug("prepare_msg() returned %d\n", status0);

	status1 = handle_msg(recvdMsg->msgid, recvdMsg);
	if (status1 < 0)
		evm_log_debug("handle_msg() returned %d\n", status1);

	status2 = finalize_msg(recvdMsg->msgid, recvdMsg);
	if (status2 < 0)
		evm_log_debug("finalize_msg() returned %d\n", status2);

	return (status0 | status1 | status2);
}

static int handle_tmr(evm_tmrid_struct *tmrid, void *tmr)
{
	if (tmrid != NULL)
		if (tmrid->tmr_handle != NULL)
			return tmrid->tmr_handle(tmr);
	return -1;
}

static int finalize_tmr(evm_tmrid_struct *tmrid, void *tmr)
{
	if (tmrid != NULL)
		if (tmrid->tmr_finalize != NULL)
			return tmrid->tmr_finalize(tmr);
	return -1;
}

static int handle_timer(evm_timer_struct *expdTmr)
{
	int status1 = 0;
	int status2 = 0;

	if (expdTmr == NULL)
		return -1;

	if (expdTmr->stopped == 0) {
		status1 = handle_tmr(expdTmr->tmrid, expdTmr);
		if (status1 < 0)
			evm_log_debug("handle_tmr() returned %d\n", status1);
	}

	status2 = finalize_tmr(expdTmr->tmrid, expdTmr);
	if (status2 < 0)
		evm_log_debug("finalize_tmr() returned %d\n", status2);

	return (status1 | status2);
}

