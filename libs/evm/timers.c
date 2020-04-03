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

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

/* global timer ID */
static timer_t global_timerid;
static tmrs_queue_struct global_tmrs_queue;
static int global_timer_created = EVM_FALSE;
static pthread_mutex_t global_timer_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Thread-specific data related. */
static int tmr_thr_keys_created = EVM_FALSE;
static pthread_key_t tmr_thr_overruns_key; /*initialized only once - not for each call to evm_init() within a process*/

static void timer_sighandler(int signum, siginfo_t *siginfo, void *context);
static int tmr_enqueue(evm_consumer_struct *consumer, evm_timer_struct *tmr);
static evm_timer_struct * tmr_dequeue(evm_consumer_struct *consumer);
static int timer_pass(evm_consumer_struct *consumer, evm_timer_struct *tmr);

static void timer_sighandler(int signum, siginfo_t *siginfo, void *context)
{
	if (signum == SIG) {
		int *thr_overruns = (int *)pthread_getspecific(tmr_thr_overruns_key);
		timer_t *timerid = siginfo->si_value.sival_ptr;

		*thr_overruns = timer_getoverrun(*timerid);
#if 0
		*thr_overruns = 15434654;
#endif
	}
}

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
	struct sigaction sact;
	struct sigevent sev;
	sigset_t sigmask;
	evm_log_info("(entry)\n");

	if (consumer == NULL) {
		evm_log_error("Event machine consumer undefined!\n");
		return NULL;
	}

	/* Prepare thread-specific integer for thread related signal post-handling. */
	evm_log_debug("tmr_thr_keys_created=%d\n", (int)tmr_thr_keys_created);
	if (!tmr_thr_keys_created) {
		if ((errno = pthread_key_create(&tmr_thr_overruns_key, NULL)) != 0) {
			evm_log_system_error("pthread_key_create() - tmr_thr_overruns_key\n");
			return NULL;
		}
		tmr_thr_keys_created = EVM_TRUE;
	}
	if ((ptr = calloc(1, sizeof(int))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): thread-specific data\n");
		return NULL;
	}
	if ((errno = pthread_setspecific(tmr_thr_overruns_key, ptr)) != 0) {
		evm_log_system_error("pthread_setspecific()\n");
		free(ptr);
		ptr = NULL;
		return NULL;
	}
	*(int *)ptr = -2; /*initialize*/

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
	consumer->tmrs_queue->last_tmr = NULL;
	pthread_mutex_init(&consumer->tmrs_queue->access_mutex, NULL);
	pthread_mutex_unlock(&consumer->tmrs_queue->access_mutex);

	pthread_mutex_lock(&global_timer_mutex);
	evm_log_debug("global_timer_created=%d\n", global_timer_created);
	if (!global_timer_created) {
		/* Block timer signal temporarily */
		evm_log_debug("Blocking signal %d\n", SIG);
		sigemptyset(&sigmask);
		sigaddset(&sigmask, SIG);
		if (pthread_sigmask(SIG_BLOCK, &sigmask, NULL) < 0) {
			evm_log_system_error("pthread_sigmask() SIG_BLOCK\n");
			pthread_mutex_unlock(&global_timer_mutex);
			free(tmrs_queue);
			tmrs_queue = NULL;
			free(ptr);
			ptr = NULL;
			return NULL;
		}

		/* Establish handler for timer signal */
		evm_log_debug("Establishing handler for signal %d\n", SIG);
		sact.sa_flags = SA_SIGINFO;
		sact.sa_sigaction = timer_sighandler;
		sigemptyset(&sact.sa_mask);
		if (sigaction(SIG, &sact, NULL) == -1) {
			evm_log_system_error("sigaction()\n");
			pthread_mutex_unlock(&global_timer_mutex);
			free(tmrs_queue);
			tmrs_queue = NULL;
			free(ptr);
			ptr = NULL;
			return NULL;
		}

		/* Create the timer */
//		sev.sigev_notify = SIGEV_SIGNAL;
		sev.sigev_notify = SIGEV_THREAD_ID;
//		sev.sigev_notify_thread_id = syscall(SYS_gettid);
		sev._sigev_un._tid = syscall(SYS_gettid);
		sev.sigev_signo = SIG;
		sev.sigev_value.sival_ptr = &global_timerid;
		if (timer_create(CLOCKID, &sev, &global_timerid) == -1) {
			evm_log_system_error("timer_create() timer ID=%p\n", (void *)global_timerid);
			pthread_mutex_unlock(&global_timer_mutex);
			free(tmrs_queue);
			tmrs_queue = NULL;
			free(ptr);
			ptr = NULL;
			return NULL;
		}

		evm_log_debug("Internal timers ID is %p\n", (void *)global_timerid);

		evm_log_debug("Unblocking signal %d\n", SIG);
		if (pthread_sigmask(SIG_UNBLOCK, &sigmask, NULL) < 0) {
			evm_log_system_error("pthread_sigmask() SIG_UNBLOCK\n");
			pthread_mutex_unlock(&global_timer_mutex);
			free(tmrs_queue);
			tmrs_queue = NULL;
			free(ptr);
			ptr = NULL;
			return NULL;
		}
		global_timer_created = EVM_TRUE;
	}
	pthread_mutex_unlock(&global_timer_mutex);

	return tmrs_queue;
}

static int tmr_enqueue(evm_consumer_struct *consumer, evm_timer_struct *tmr)
{
	int rv = 0;
	tmrs_queue_struct *tmrs_queue;
	sem_t *bsem;
	pthread_mutex_t *amtx;
	evm_log_info("(entry)\n");

	if (consumer != NULL) {
		tmrs_queue = consumer->tmrs_queue;
		bsem = &consumer->blocking_sem;
		if (tmrs_queue != NULL)
			amtx = &tmrs_queue->access_mutex;
		else
			rv = -1;
	} else
		rv = -1;

	if (rv == 0) {
		pthread_mutex_lock(amtx);
		if (tmrs_queue->last_tmr == NULL)
			tmrs_queue->first_tmr = tmr;
		else
			tmrs_queue->last_tmr->next = tmr;

		tmrs_queue->last_tmr = tmr;
		tmr->next = NULL;
		pthread_mutex_unlock(amtx);
		evm_log_info("Post blocking semaphore (UNBLOCK)\n");
		sem_post(bsem);
	}
	return rv;
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
	if (tmr->next == NULL) {
		tmrs_queue->first_tmr = NULL;
		tmrs_queue->last_tmr = NULL;
	} else
		tmrs_queue->first_tmr = tmr->next;

	tmr->consumer = consumer; /*just in case:)*/
	pthread_mutex_unlock(amtx);
	return tmr;
}

/*
 * Internal passing of expired timers from global queue
 * to its internal tmrs_queue when expired time doesn't belong
 * to running evn_timers_check() (different consumer).
 */
static int timer_pass(evm_consumer_struct *consumer, evm_timer_struct *tmr)
{
	evm_log_info("(entry) consumer=%p, msg=%p\n", consumer, tmr);
	if (consumer != NULL) {
		if (tmr_enqueue(consumer, tmr) != 0) {
			evm_log_error("Timer enqueuing failed!\n");
			return -1;
		}
		return 0;
	}
	return -1;
}

evm_timer_struct * timers_check(evm_consumer_struct *consumer)
{
	evm_timer_struct *tmr_return = NULL;
	evm_timer_struct *tmr;
	struct timespec time_stamp;
	struct itimerspec its;
	int *thr_overruns;
	evm_log_info("(entry)\n");

	if (consumer == NULL)
		return NULL;
	evm_log_info("(entry) consumer=%p\n",consumer);

	if (consumer->tmrs_queue == NULL)
		return NULL;

	tmr = consumer->tmrs_queue->first_tmr;
	evm_log_debug("(entry) tmr=%p\n", tmr);
	if (tmr != NULL)
		return tmr_dequeue(consumer); /*first consume already prepared expired timers*/

	/*Check the global timers queue for the newly expired timers.*/
	pthread_mutex_lock(&global_timer_mutex);
	tmr = global_tmrs_queue.first_tmr;
	if (tmr == NULL) {
		pthread_mutex_unlock(&global_timer_mutex);
		return NULL; /*no timers set*/
	}

	thr_overruns = (int *)pthread_getspecific(tmr_thr_overruns_key);
	if (thr_overruns != NULL) {
		if (*thr_overruns < 0) {
			if (*thr_overruns == -1) {
				evm_log_debug("Error getting timer thread overruns!\n");
			}
		} else {
			if (*thr_overruns > 0) {
				evm_log_debug("timer thread overruns=%d\n", *thr_overruns);
			}
		}
		*thr_overruns = -2;
	}

	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;

	if (clock_gettime(CLOCK_MONOTONIC, &time_stamp) == -1) {
		evm_log_system_error("clock_gettime()\n");
		/* on clock_gettime() failure reschedule timers_check() to next second */
		its.it_value.tv_sec = 1;
		if (timer_settime(global_timerid, 0, &its, NULL) == -1) {
			evm_log_system_error("timer_settime timer ID=%p\n", (void *)global_timerid);
		}
		pthread_mutex_unlock(&global_timer_mutex);
		return NULL;
	}

	if (
		(time_stamp.tv_sec > tmr->tm_stamp.tv_sec) ||
		(
			(time_stamp.tv_sec == tmr->tm_stamp.tv_sec) &&
			(time_stamp.tv_nsec >= tmr->tm_stamp.tv_nsec)
		)
	) {
		global_tmrs_queue.first_tmr = tmr->next;
//		evm_log_debug("timer expired: tmr=%p, stopped=%d\n", (void *)tmr, tmr->stopped);

		tmr_return = tmr;
		tmr = global_tmrs_queue.first_tmr;
	}

	if (tmr != NULL) {
//		evm_log_debug("next(sec)=%ld, stamp(sec)=%ld, next(nsec)=%ld, stamp(nsec)=%ld\n", tmr->tm_stamp.tv_sec, time_stamp.tv_sec, tmr->tm_stamp.tv_nsec, time_stamp.tv_nsec);
		if (tmr->tm_stamp.tv_sec > time_stamp.tv_sec) {
			its.it_value.tv_sec = tmr->tm_stamp.tv_sec - time_stamp.tv_sec;
			if (tmr->tm_stamp.tv_nsec > time_stamp.tv_nsec)
				its.it_value.tv_nsec = tmr->tm_stamp.tv_nsec - time_stamp.tv_nsec;
			else {
				its.it_value.tv_nsec = (long)1000000000 + tmr->tm_stamp.tv_nsec - time_stamp.tv_nsec;
				its.it_value.tv_sec--;
			}
		} else if (tmr->tm_stamp.tv_sec == time_stamp.tv_sec) {
			if (tmr->tm_stamp.tv_nsec > time_stamp.tv_nsec)
				its.it_value.tv_nsec = tmr->tm_stamp.tv_nsec - time_stamp.tv_nsec;
#if 0 /* This part is already provided via "its" initialization. */
			else {
				its.it_value.tv_sec = 0;
				its.it_value.tv_nsec = 0;
			}
		} else {
			its.it_value.tv_sec = 0;
			its.it_value.tv_nsec = 0;
#endif
		}

//		evm_log_debug("its(sec)=%ld, its(nsec)=%ld\n", its.it_value.tv_sec, its.it_value.tv_nsec);
		if ((its.it_value.tv_sec != 0) || (its.it_value.tv_nsec != 0)) {
			if (timer_settime(global_timerid, 0, &its, NULL) == -1) {
				evm_log_system_error("timer_settime timer ID=%p\n", (void *)global_timerid);
			}
		} else {
			evm_log_debug("its(sec)=%ld, its(nsec)=%ld, tmr_return=%p\n", its.it_value.tv_sec, its.it_value.tv_nsec, tmr_return);
//			abort();
		}
	}
	pthread_mutex_unlock(&global_timer_mutex);

	if (tmr_return != NULL) {
		if (tmr_return->consumer == consumer)
			return tmr_return;

		if (timer_pass(tmr_return->consumer, tmr_return) < 0) {
			evm_log_error("Timer passing failed!\n");
		}
	}

	return NULL;
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
	struct itimerspec its;
	evmTimerStruct *new;
	evm_timer_struct *prev, *tmr;
	evm_log_info("(entry)\n");

	if (consumer == NULL) {
		evm_log_error("Event machine consumer object undefined!\n");
		return NULL;
	}

	if (tmrid == NULL) {
		evm_log_error("Event machine tmrid object undefined!\n");
		return NULL;
	}

	new = (evmTimerStruct *)calloc(1, sizeof(evmTimerStruct));
	if (new == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	evm_log_debug("New timer ptr=%p\n", (void *)new);

	pthread_mutex_init(&new->amtx, NULL);
	pthread_mutex_unlock(&new->amtx);
	pthread_mutex_lock(&new->amtx);
	new->tmrid = tmrid;
	new->consumer = consumer;
	new->saved = 0;
	new->stopped = 0;
	new->ctx = ctx;

	if (clock_gettime(CLOCK_MONOTONIC, &new->tm_stamp) == -1) {
		evm_log_system_error("clock_gettime()\n");
		free(new);
		new = NULL;
		return NULL;
	}

	its.it_value.tv_sec = tv_sec;
	its.it_value.tv_nsec = tv_nsec;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;

	new->tm_stamp.tv_sec += its.it_value.tv_sec;
	new->tm_stamp.tv_nsec += its.it_value.tv_nsec;

	/* If no timers set, we are the first to expire -> start it!*/
	pthread_mutex_lock(&global_timer_mutex);
	tmr = global_tmrs_queue.first_tmr;
	if (tmr == NULL) {
		if (timer_settime(global_timerid, 0, &its, NULL) == -1) {
			evm_log_system_error("timer_settime timer ID=%p\n", (void *)global_timerid);
			free(new);
			new = NULL;
			pthread_mutex_unlock(&global_timer_mutex);
			return NULL;
		}

		global_tmrs_queue.first_tmr = new;
		pthread_mutex_unlock(&global_timer_mutex);
		pthread_mutex_unlock(&new->amtx);
		return new;
	}

	prev = tmr;
	while (tmr != NULL) {
		if (
			(new->tm_stamp.tv_sec < tmr->tm_stamp.tv_sec) ||
			(
				(new->tm_stamp.tv_sec == tmr->tm_stamp.tv_sec) &&
				(new->tm_stamp.tv_nsec < tmr->tm_stamp.tv_nsec)
			)
		) {
			new->next = tmr;
			/* If first in the list, we are the first to expire -> start it!*/
			if (tmr == global_tmrs_queue.first_tmr) {
				if (timer_settime(global_timerid, 0, &its, NULL) == -1) {
					evm_log_system_error("timer_settime timer ID=%p\n", (void *)global_timerid);
					free(new);
					new = NULL;
					pthread_mutex_unlock(&global_timer_mutex);
					return NULL;
				}
				global_tmrs_queue.first_tmr = new;
			} else
				prev->next = new;

			pthread_mutex_unlock(&global_timer_mutex);
			pthread_mutex_unlock(&new->amtx);
			return new;
		}

		prev = tmr;
		tmr = tmr->next;
	}

	/* The last in the list! */
	prev->next = new;
	pthread_mutex_unlock(&global_timer_mutex);
	pthread_mutex_unlock(&new->amtx);

	return new;
}

int evm_timer_stop(evmTimerStruct *tmr)
{
	evmTimerStruct *curr_tmr = NULL;
	evmTimerStruct *prev_tmr = NULL;
	evm_log_info("(entry)\n");

	if (tmr == NULL)
		return -1;

	pthread_mutex_lock(&global_timer_mutex);
	curr_tmr = global_tmrs_queue.first_tmr;
	while (curr_tmr != NULL) {
		if (curr_tmr == tmr) {
			/* started timer "tmr" found - remove it from the global timers list */
			pthread_mutex_lock(&tmr->amtx);
			tmr->stopped = 1;

			if (prev_tmr == NULL)
				global_tmrs_queue.first_tmr = tmr->next;
			else
				prev_tmr = tmr->next;

			tmr->next = NULL;
			pthread_mutex_unlock(&tmr->amtx);
			pthread_mutex_unlock(&global_timer_mutex);
			return 0;
		}
		prev_tmr = curr_tmr;
		curr_tmr = curr_tmr->next;
	}

	pthread_mutex_unlock(&global_timer_mutex);
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

