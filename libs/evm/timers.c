/*
 * The EVM timers module
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

#ifndef EVM_FILE_timers_c
#define EVM_FILE_timers_c
#else
#error Preprocesor macro EVM_FILE_timers_c conflict!
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>

#include "evm/libevm.h"
#include <sys/eventfd.h>
#include "evm.h"
#include "timers.h"

#include "userlog/log_module.h"
EVMLOG_MODULE_INIT(EVM_TMRS, 1)

static evm_timer_struct * tmr_dequeue(evm_consumer_struct *consumer);

/*
 * Per consumer timers initialization.
 * Return:
 * - Pointer to initialized tmrs_queue
 * - NULL on failure
 */
tmrs_queue_struct * timers_queue_init(evm_consumer_struct *consumer)
{
	void *ptr = NULL;
	tmrs_queue_struct *tmrs_queue = NULL;
	evm_log_info("(entry)\n");

	if (consumer == NULL) {
		evm_log_error("Event machine consumer undefined!\n");
		return NULL;
	}

	/* Setup consumer internal timer queue. */
	if ((tmrs_queue = (tmrs_queue_struct *)calloc(1, sizeof(tmrs_queue_struct))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): internal timer queue\n");
		free(ptr);
		ptr = NULL;
		return NULL;
	}
	consumer->tmrs_queue = tmrs_queue;
	consumer->tmrs_queue->first_tmr = NULL;
	pthread_mutex_init(&consumer->tmrs_queue->access_mutex, NULL);
	pthread_mutex_unlock(&consumer->tmrs_queue->access_mutex);

	return tmrs_queue;
}

static evm_timer_struct * tmr_dequeue(evm_consumer_struct *consumer)
{
	evm_timer_struct *tmr;
	tmrs_queue_struct *tmrs_queue;
	pthread_mutex_t *amtx;
	evm_log_info("(entry) consumer=%p\n", consumer);

	if (consumer != NULL) {
		tmrs_queue = consumer->tmrs_queue;
		if (tmrs_queue != NULL)
			amtx = &tmrs_queue->access_mutex;
		else
			return NULL;
	} else
		return NULL;

	pthread_mutex_lock(amtx);
	evm_log_debug("tmrs_queue=%p\n", tmrs_queue);
	tmr = tmrs_queue->first_tmr;
	if (tmr == NULL) {
		pthread_mutex_unlock(amtx);
		return NULL;
	}

	evm_log_debug("tmr=%p\n", tmr);
	tmrs_queue->first_tmr = tmr->next;

	tmr->consumer = consumer; /*just in case:)*/
	pthread_mutex_unlock(amtx);
	return tmr;
}

evm_timer_struct * timers_check(evm_consumer_struct *consumer)
{
	evm_timer_struct *tmr;
	struct timespec time_stamp;
	evm_log_info("(entry)\n");

	if (consumer == NULL)
		return NULL;
	evm_log_info("(entry) consumer=%p\n",consumer);

	if (consumer->tmrs_queue == NULL)
		return NULL;

	tmr = consumer->tmrs_queue->first_tmr;
	evm_log_debug("(entry) tmr=%p\n", tmr);
	if (tmr != NULL) {
		pthread_mutex_lock(&tmr->amtx);
		if (clock_gettime(CLOCK_REALTIME, &time_stamp) == -1) {
			evm_log_system_error("clock_gettime()\n");
			pthread_mutex_unlock(&tmr->amtx);
			return NULL;
		}
		if (
			(time_stamp.tv_sec > tmr->tm_stamp.tv_sec) ||
			(
				(time_stamp.tv_sec == tmr->tm_stamp.tv_sec) &&
				(time_stamp.tv_nsec >= tmr->tm_stamp.tv_nsec)
			)
		) {
			evm_log_debug("next(sec)=%ld, stamp(sec)=%ld, next(nsec)=%ld, stamp(nsec)=%ld\n", tmr->tm_stamp.tv_sec, time_stamp.tv_sec, tmr->tm_stamp.tv_nsec, time_stamp.tv_nsec);
			pthread_mutex_unlock(&tmr->amtx);
			return tmr_dequeue(consumer); /* Timer expired! */
		}
		pthread_mutex_unlock(&tmr->amtx);
	}

	return NULL;
}

struct timespec * timers_next_ts(evm_consumer_struct *consumer)
{
	struct timespec *ts;
	evm_timer_struct *tmr;
	tmrs_queue_struct *tmrs_queue;
	pthread_mutex_t *amtx;
	evm_log_info("(entry) consumer=%p\n", consumer);

	if (consumer != NULL) {
		tmrs_queue = consumer->tmrs_queue;
		if (tmrs_queue != NULL)
			amtx = &tmrs_queue->access_mutex;
		else
			return NULL;
	} else
		return NULL;

	pthread_mutex_lock(amtx);
	evm_log_debug("tmrs_queue=%p\n", tmrs_queue);
	tmr = tmrs_queue->first_tmr;
	evm_log_debug("tmr=%p\n", tmr);
	if (tmr == NULL) {
		pthread_mutex_unlock(amtx);
		evm_log_debug("No timers set!\n");
		return NULL;
	}

	ts = &tmr->tm_stamp;
	pthread_mutex_unlock(amtx);
	return ts;
}

/*
 * Public API functions:
 * - evm_tmrid_add()
 * - evm_tmrid_get()
 * - evm_tmrid_del()
 */
evmTmridStruct * evm_tmrid_add(evmStruct *evm, int id)
{
	evmTmridStruct *tmrid = NULL;
	evmlist_el_struct *tmp, *new;
	evm_log_info("(entry)\n");

	if (evm != NULL) {
		if (evm->tmrids_list != NULL) {
			pthread_mutex_lock(&evm->tmrids_list->access_mutex);
			tmp = evm_search_evmlist(evm->tmrids_list, id);
			if ((tmp != NULL) && (tmp->id == id)) {
				/* required id already exists - return existing element */
				tmrid = (evm_tmrid_struct *)tmp->el;
			} else {
				/* create new evmlist element with id */
				if ((new = evm_new_evmlist_el(id)) != NULL) {
					/* create new tmrid */
					if ((tmrid = (evm_tmrid_struct *)calloc(1, sizeof(evm_tmrid_struct))) == NULL) {
						errno = ENOMEM;
						evm_log_system_error("calloc(): tmrid\n");
						free(new);
						new = NULL;
					}
					if (tmrid != NULL) {
						tmrid->evm = evm;
						tmrid->id = id;
						tmrid->tmr_handle = NULL;
					}
				}
				if (new != NULL) {
					new->id = id;
					new->el = (void *)tmrid;
					new->prev = tmp;
					new->next = NULL;
					if (tmp != NULL)
						tmp->next = new;
					else
						evm->tmrids_list->first = new;
				}
			}
			pthread_mutex_unlock(&evm->tmrids_list->access_mutex);
		}
	}
	return tmrid;
}

evmTmridStruct * evm_tmrid_get(evmStruct *evm, int id)
{
	evmTmridStruct *tmrid = NULL;
	evmlist_el_struct *tmp;
	evm_log_info("(entry)\n");

	if (evm != NULL) {
		if (evm->tmrids_list != NULL) {
			pthread_mutex_lock(&evm->tmrids_list->access_mutex);
			tmp = evm_search_evmlist(evm->tmrids_list, id);
			evm_log_debug("tmp=%p\n", tmp);
			if ((tmp != NULL) && (tmp->id == id)) {
				/* required id already exists - return existing element */
				tmrid = (evm_tmrid_struct *)tmp->el;
			}
			pthread_mutex_unlock(&evm->tmrids_list->access_mutex);
		}
	}
	evm_log_debug("tmrid=%p\n", tmrid);
	return tmrid;
}

evmTmridStruct * evm_tmrid_del(evmStruct *evm, int id)
{
	evmTmridStruct *tmrid = NULL;
	evmlist_el_struct *tmp;
	evm_log_info("(entry)\n");

	if (evm != NULL) {
		if (evm->tmrids_list != NULL) {
			pthread_mutex_lock(&evm->tmrids_list->access_mutex);
			tmp = evm_search_evmlist(evm->tmrids_list, id);
			if ((tmp != NULL) && (tmp->id == id)) {
				/* required id already exists - return existing element */
				tmrid = (evm_tmrid_struct *)tmp->el;
				if (tmrid != NULL) {
					free(tmrid);
				}
				tmp->prev->next = tmp->next;
				if (tmp->next != NULL)
					tmp->next->prev = tmp->prev;
				free(tmp);
				tmp = NULL;
			}
			pthread_mutex_unlock(&evm->tmrids_list->access_mutex);
		}
	}
	return tmrid;
}

/*
 * Public API functions:
 * - evm_tmrid_cb_handle_set()
 */
int evm_tmrid_cb_handle_set(evmTmridStruct *tmrid, int (*tmr_handle)(evmConsumerStruct *consumer, evmTimerStruct *tmr))
{
	int rv = 0;
	evm_log_info("(entry)\n");

	if (tmrid == NULL)
		return -1;

	if (tmr_handle == NULL)
		return -1;

	if (rv == 0) {
		tmrid->tmr_handle = tmr_handle;
	}
	return rv;
}

/*
 * Public API functions:
 * - evm_timer_start()
 * - evm_timer_stop()
 * - evm_timer_ctx_set()
 * - evm_timer_ctx_get()
 */
evmTimerStruct * evm_timer_start(evmConsumerStruct *consumer, evmTmridStruct *tmrid, time_t tv_sec, long tv_nsec, void *ctx)
{
	evmTimerStruct *new;
	evm_timer_struct *prev, *tmr;
	tmrs_queue_struct *tmrs_queue;
	pthread_mutex_t *tmrs_queue_amtx;
	evm_log_info("(entry) consumer=%p\n", consumer);

	if (consumer == NULL) {
		evm_log_error("Event machine consumer object undefined!\n");
		return NULL;
	}

	if (tmrid == NULL) {
		evm_log_error("Event machine tmrid object undefined!\n");
		return NULL;
	}

	tmrs_queue = consumer->tmrs_queue;
	if (tmrs_queue != NULL)
		tmrs_queue_amtx = &tmrs_queue->access_mutex;
	else
		return NULL;

	new = (evmTimerStruct *)calloc(1, sizeof(evmTimerStruct));
	if (new == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	pthread_mutex_init(&new->amtx, NULL);
	pthread_mutex_unlock(&new->amtx);
	new->tmrid = tmrid;
	new->consumer = consumer;
	new->saved = 0;
	new->stopped = 0;
	new->ctx = ctx;

	if (clock_gettime(CLOCK_REALTIME, &new->tm_stamp) == -1) {
		evm_log_system_error("clock_gettime()\n");
		free(new);
		new = NULL;
		return NULL;
	}

	new->tm_stamp.tv_sec += tv_sec;
	new->tm_stamp.tv_nsec += tv_nsec;

	evm_log_debug("New timer: ptr=%p, new(sec)=%ld, new(nsec)=%ld\n", (void *)new, new->tm_stamp.tv_sec, new->tm_stamp.tv_nsec);

	pthread_mutex_lock(tmrs_queue_amtx);
	pthread_mutex_lock(&new->amtx);
	evm_log_debug("tmrs_queue=%p\n", tmrs_queue);
	tmr = tmrs_queue->first_tmr;
	evm_log_debug("tmr=%p\n", tmr);
	prev = NULL;
	do {
		if (
			(tmr == NULL) ||
			(new->tm_stamp.tv_sec < tmr->tm_stamp.tv_sec) ||
			(
				(new->tm_stamp.tv_sec == tmr->tm_stamp.tv_sec) &&
				(new->tm_stamp.tv_nsec < tmr->tm_stamp.tv_nsec)
			)
		) {
			new->next = tmr;
			/* If first in the list, we are the first to expire -> start it!*/
			if (prev == NULL) {
				tmrs_queue->first_tmr = new;
				evm_log_debug("New timer: ptr=%p (first to expire at setup)\n", (void *)new);
				pthread_mutex_unlock(&new->amtx);
				pthread_mutex_unlock(tmrs_queue_amtx);
				return new;
			}
			break;
		}

		prev = tmr;
		tmr = tmr->next;
	} while (tmr != NULL);

	/* Not the first in the list! */
	prev->next = new;
	evm_log_debug("New timer: ptr=%p (not the first to expire at setup)\n", (void *)new);
	pthread_mutex_unlock(&new->amtx);
	pthread_mutex_unlock(tmrs_queue_amtx);

	return new;
}

int evm_timer_stop(evmTimerStruct *tmr)
{
	evmTimerStruct *curr = NULL;
	evmTimerStruct *prev = NULL;
	evmConsumerStruct *consumer = NULL;
	tmrs_queue_struct *tmrs_queue;
	pthread_mutex_t *tmrs_queue_amtx;
	evm_log_info("(entry) tmr=%p\n", tmr);

	if (tmr == NULL) {
		evm_log_debug("Stopping NULL timer!\n");
		return -1;
	}

	if ((consumer = tmr->consumer) == NULL) {
		evm_log_debug("Stopping timer without consumer set!\n");
		return -1;
	}

	tmrs_queue = consumer->tmrs_queue;
	if (tmrs_queue != NULL)
		tmrs_queue_amtx = &tmrs_queue->access_mutex;
	else {
		evm_log_debug("Stopping timer for consumer without its timer queue!\n");
		return -1;
	}

	pthread_mutex_lock(tmrs_queue_amtx);
	curr = tmrs_queue->first_tmr;
	while (curr != NULL) {
		if (curr == tmr) {
			/* started timer "tmr" found - remove it from the global timers list */
			pthread_mutex_lock(&tmr->amtx);
			tmr->stopped = 1;

			if (prev == NULL)
				tmrs_queue->first_tmr = curr->next;
			else
				prev->next = curr->next;

			tmr->next = NULL;
			pthread_mutex_unlock(&tmr->amtx);
			pthread_mutex_unlock(tmrs_queue_amtx);
			return 0;
		}
		prev = curr;
		curr = curr->next;
	}

	pthread_mutex_unlock(tmrs_queue_amtx);
	return -1;
}

int evm_timer_ctx_set(evmTimerStruct *tmr, void *ctx)
{
	evm_log_info("(entry)\n");

	if (tmr == NULL)
		return -1;

	if (ctx == NULL)
		return -1;

	pthread_mutex_lock(&tmr->amtx);
	tmr->ctx = ctx;
	pthread_mutex_unlock(&tmr->amtx);
	return 0;
}

void * evm_timer_ctx_get(evmTimerStruct *tmr)
{
	void *ctx;
	evm_log_info("(entry)\n");

	if (tmr == NULL)
		return NULL;

	pthread_mutex_lock(&tmr->amtx);
	ctx = tmr->ctx;
	pthread_mutex_unlock(&tmr->amtx);
	return ctx;
}

/*
 * Public API function:
 * - evm_timer_delete()
 */
void evm_timer_delete(evmTimerStruct *tmr)
{
	evm_log_info("(entry) tmr=%p\n", tmr);

	if (tmr != NULL) {
		pthread_mutex_lock(&tmr->amtx);
		if (tmr->saved == 0) {
			free(tmr);
			tmr = NULL; /*required only, if to be used below in this function*/
		} else {
			pthread_mutex_unlock(&tmr->amtx);
		}
	}
}

