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
#include <string.h>
#include <sys/types.h>
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

static int handle_timer(evm_consumer_struct *consumer, evm_timer_struct *expd_tmr);
static int handle_message(evm_consumer_struct *consumer, evm_message_struct *rcvd_msg);

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

evmlist_el_struct * evm_check_evmlist(evmlist_head_struct *head, void *el)
{
	evmlist_el_struct *tmp = NULL;
	evm_log_info("(entry) head=%p\n", head);

	if (el == NULL)
		return NULL;

	if (head != NULL) {
		tmp = head->first;
		while (tmp != NULL) {
			if (tmp->next == NULL)
				break;
			if (tmp->el == el)
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
				/* required id already exists - delete existing element */
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
					}
					if (topic != NULL) {
						if ((topic->consumers_list = calloc(1, sizeof(evmlist_head_struct))) == NULL) {
							free(topic);
							topic = NULL;
							free(new);
							new = NULL;
						} else {
							pthread_mutex_init(&topic->consumers_list->access_mutex, NULL);
							pthread_mutex_unlock(&topic->consumers_list->access_mutex);
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
				/* required id already exists - delete existing element */
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
 * Internal consumer topic addition and removal funstions:
 * topic_consumer_add()
 * topic_consumer_del()
 */
/*
 * Function: topic_consumer_add()
 * Returns:
 * - NULL, if:
 *   - any of parameters is NULL
 *   - topic's consumer unsuccessfully added
 * - Same consumer pointer provided as parameter, when:
 *   - topic's consumer successully added
 *   - topic's consumer already added
 *
 *   If really added, then per topic consumers_list size increments!
 */
static evm_consumer_struct * topic_consumer_add(evm_topic_struct *topic, evm_consumer_struct *consumer)
{
	evmlist_el_struct *tmp, *new;
	evm_log_info("(entry)\n");

	if ((topic != NULL) && (consumer != NULL)) {
		if (topic->consumers_list != NULL) {
			pthread_mutex_lock(&topic->consumers_list->access_mutex);
			tmp = evm_check_evmlist(topic->consumers_list, (void *)consumer);
			if ((tmp == NULL) || (tmp->el != (void *)consumer)) {
				/* List is empty or element not yet present */
				/* create new evmlist element with id */
				if ((new = evm_new_evmlist_el(consumer->id)) != NULL) {
					/* add supplied consumer */
					new->el = (void *)consumer;
					new->prev = tmp;
					new->next = NULL;
					if (tmp != NULL)
						tmp->next = new;
					else
						topic->consumers_list->first = new;
				} else
					consumer = NULL;
			}
			pthread_mutex_unlock(&topic->consumers_list->access_mutex);
		}
	} else
		consumer = NULL;

	return consumer;
}

/*
 * Function: topic_consumer_del()
 * Returns:
 * - NULL, if;
 *   - any of parameters is NULL
 * - Same consumer pointer as provided in parameter, when:
 *   - topic's consumer successully removed
 *   - topic's consumer not found (already removed)
 */
static evm_consumer_struct * topic_consumer_del(evm_topic_struct *topic, evm_consumer_struct *consumer)
{
	evmlist_el_struct *tmp;
	evm_log_info("(entry)\n");

	if ((topic != NULL) && (consumer != NULL)) {
		if (topic->consumers_list != NULL) {
			pthread_mutex_lock(&topic->consumers_list->access_mutex);
			tmp = evm_check_evmlist(topic->consumers_list, (void *)consumer);
			if ((tmp != NULL) && (tmp->el == (void *)consumer)) {
				/* List is not empty and element present */
				/* Delete evmlist element */
				tmp->prev->next = tmp->next;
				if (tmp->next != NULL)
					tmp->next->prev = tmp->prev;
				free(tmp);
				tmp = NULL;
			}
			pthread_mutex_unlock(&topic->consumers_list->access_mutex);
		}
	} else
		consumer = NULL;

	return consumer;
}

/*
 * Public API functions:
 * - evm_topic_subscribe()
 * - evm_topic_unsubscribe()
 */
evmTopicStruct * evm_topic_subscribe(evmConsumerStruct *consumer, int topic_id)
{
	evmStruct *evm = NULL;
	evmTopicStruct *topic = NULL;
	evmlist_el_struct *tmp;
	evm_log_info("(entry)\n");

	if (consumer == NULL)
		return NULL;

	evm = consumer->evm;
	if (evm == NULL)
		return NULL;

	if (evm->topics_list == NULL)
		return NULL;

	pthread_mutex_lock(&evm->topics_list->access_mutex);
	tmp = evm_search_evmlist(evm->topics_list, topic_id);
	if ((tmp != NULL) && (tmp->id == topic_id)) {
		/* required id found - register topic element */
		topic = (evm_topic_struct *)tmp->el;
	} else
		topic = NULL;
	if (topic != NULL) {
		if ((consumer = topic_consumer_add(topic, consumer)) == NULL) {
			topic = NULL;
		}
	}

	pthread_mutex_unlock(&evm->topics_list->access_mutex);

	return topic;
}

evmTopicStruct * evm_topic_unsubscribe(evmConsumerStruct *consumer, int id)
{
	evmStruct *evm = NULL;
	evmTopicStruct *topic = NULL;
	evmlist_el_struct *tmp;
	evm_log_info("(entry)\n");

	if (consumer == NULL)
		return NULL;

	evm = consumer->evm;
	if (evm == NULL)
		return NULL;

	if (evm->topics_list == NULL)
		return NULL;

	pthread_mutex_lock(&evm->topics_list->access_mutex);
	tmp = evm_search_evmlist(evm->topics_list, id);
	if ((tmp != NULL) && (tmp->id == id)) {
		/* required id found - unregister topic element */
		topic = (evm_topic_struct *)tmp->el;
	} else
		topic = NULL;
	if (topic != NULL) {
		topic_consumer_del(topic, consumer);
	}

	pthread_mutex_unlock(&evm->topics_list->access_mutex);

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
	int rv = 0;
	evm_timer_struct *expd_tmr;
	evm_message_struct *rcvd_msg;
	struct timespec *ts;
	evm_log_info("(entry)\n");

	if (consumer == NULL) {
		evm_log_error("Event machine consumer object undefined!\n");
		abort();
	}

	/* Loop exclusively over expired timers (non-blocking already)! */
	for (;;) {
		evm_log_info("(loop entry) check and handle all expired timers\n");
		/* Handle expired timer (NON-BLOCKING). */
		if ((expd_tmr = timers_check(consumer)) != NULL) {
			if ((rv = handle_timer(consumer, expd_tmr)) < 0)
				evm_log_debug("handle_timer() returned %d\n", rv);
		} else {
			ts = timers_next_ts(consumer);
			break;
		}
	}

	/* Handle handle received message (WAIT - THE ONLY POTENTIALLY BLOCKING POINT). */
	if ((rcvd_msg = messages_check(consumer, ts)) != NULL) {
		if ((rv = handle_message(consumer, rcvd_msg)) < 0)
			evm_log_debug("handle_message() returned %d\n", rv);
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
	evm_log_info("(entry)\n");

	if (consumer == NULL) {
		evm_log_error("Event machine consumer object undefined!\n");
		abort();
	}

	/* Main protocol loop! */
	for (;;) {
		evm_log_info("(main loop entry)\n");
		evm_run_once(consumer);
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

static int handle_message(evm_consumer_struct *consumer, evm_message_struct *msg)
{
	int rv = 0;
	evm_log_info("(entry)\n");

	if (consumer == NULL) {
		evm_log_debug("consumer == NULL\n");
		return -1;
	}
	if (msg == NULL) {
		evm_log_debug("msg == NULL\n");
		return -1;
	}
	if (msg->msgid == NULL) {
		evm_log_debug("msgid == NULL\n");
		return -1;
	}
	if (msg->msgid->msg_handle == NULL) {
		evm_log_debug("msg_handle == NULL\n");
		return -1;
	}
	if ((rv = msg->msgid->msg_handle(consumer, msg)) < 0)
		evm_log_debug("msg_handle returned %d\n", rv);

	evm_message_delete(msg);

	return rv;
}

static int handle_timer(evm_consumer_struct *consumer, evm_timer_struct *tmr)
{
	int rv = 0;
	evm_log_info("(entry)\n");

	if (consumer == NULL) {
		evm_log_debug("consumer == NULL\n");
		return -1;
	}
	if (tmr == NULL) {
		evm_log_debug("tmr == NULL\n");
		return -1;
	}
	if (tmr->tmrid == NULL) {
		evm_log_debug("tmrid == NULL\n");
		return -1;
	}
	if (tmr->tmrid->tmr_handle == NULL) {
		evm_log_debug("tmr_handle == NULL\n");
		return -1;
	}
	if (tmr->stopped == 0) {
		if ((rv = tmr->tmrid->tmr_handle(consumer, tmr)) < 0)
			evm_log_debug("tmr_handle returned %d\n", rv);
	}

	evm_timer_delete(tmr);

	return rv;
}

