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
#include "timers.h"

#include "userlog/log_module.h"
EVMLOG_MODULE_INIT(EVM_TMRS, 1)

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

typedef struct evm_timer_queue evm_timer_queue_struct;

struct evm_timer_queue {
	evm_timer_struct *first_tmr;
	evm_timer_struct *last_tmr;
	pthread_mutex_t access_mutex;
};

/* global timer ID */
static timer_t evm_global_timerid;
static evm_timer_queue_struct evm_global_tmr_queue;
static int evm_global_timer_not_created = 1;
static pthread_mutex_t evm_global_timer_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Thread-specific data related. */
static int evm_thr_keys_not_created = 1;
static pthread_key_t evm_thr_overruns_key; /*initialized only once - not for each call to evm_init() within a process*/

static void evm_timers_sighandler(int signum, siginfo_t *siginfo, void *context)
{
	if (signum == SIG) {
		int *thr_overruns = (int *)pthread_getspecific(evm_thr_overruns_key);
		timer_t *timerid_ptr = siginfo->si_value.sival_ptr;

		*thr_overruns = timer_getoverrun(*timerid_ptr);
#if 0
		*thr_overruns = 15434654;
#endif
	}
}

int evm_timers_init(evm_init_struct *evm_init_ptr)
{
	void *ptr;
	int status = -1;
	struct sigaction sact;
	struct sigevent sev;
	sigset_t sigmask;

	evm_log_info("(entry)\n");
	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
		return status;
	}

	/* Prepare thread-specific integer for thread related signal post-handling. */
	if (evm_thr_keys_not_created) {
		if ((errno = pthread_key_create(&evm_thr_overruns_key, NULL)) != 0) {
			evm_log_return_system_err("pthread_key_create()\n");
		}
		evm_thr_keys_not_created = 0;
	}
	if ((ptr = calloc(1, sizeof(int))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): thread-specific data\n");
		return status;
	}
	if ((errno = pthread_setspecific(evm_thr_overruns_key, ptr)) != 0) {
		evm_log_return_system_err("pthread_setspecific()\n");
	}
	*(int *)ptr = -2; /*initialize*/

	/* Setup thread specific internal timer queue. */
	if ((ptr = calloc(1, sizeof(evm_timer_queue_struct))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): internal timer queue\n");
		return status;
	}
	evm_init_ptr->tmr_queue = (evm_timer_queue_struct *)ptr;
	((evm_timer_queue_struct *)evm_init_ptr->tmr_queue)->first_tmr = NULL;
	((evm_timer_queue_struct *)evm_init_ptr->tmr_queue)->last_tmr = NULL;
	pthread_mutex_init(&((evm_timer_queue_struct *)ptr)->access_mutex, NULL);
	pthread_mutex_unlock(&((evm_timer_queue_struct *)ptr)->access_mutex);

	pthread_mutex_lock(&evm_global_timer_mutex);
	if (evm_global_timer_not_created) {
		/* Block timer signal temporarily */
		evm_log_debug("Blocking signal %d\n", SIG);
		sigemptyset(&sigmask);
		sigaddset(&sigmask, SIG);
		if (pthread_sigmask(SIG_BLOCK, &sigmask, NULL) < 0) {
			evm_log_return_system_err("pthread_sigmask() SIG_BLOCK\n");
			pthread_mutex_unlock(&evm_global_timer_mutex);
		}

		/* Establish handler for timer signal */
		evm_log_debug("Establishing handler for signal %d\n", SIG);
		sact.sa_flags = SA_SIGINFO;
		sact.sa_sigaction = evm_timers_sighandler;
		sigemptyset(&sact.sa_mask);
		if (sigaction(SIG, &sact, NULL) == -1) {
			evm_log_return_system_err("sigaction()\n");
			pthread_mutex_unlock(&evm_global_timer_mutex);
		}

		/* Create the timer */
#if 0 /*orig*/
		if ((evm_init_ptr->timerid = (timer_t *)calloc(1, sizeof(timer_t))) == NULL) {
			errno = ENOMEM;
			evm_log_system_error("calloc(): internal timer ID\n");
			pthread_mutex_unlock(&evm_global_timer_mutex);
			return status;
		}
#endif
//		sev.sigev_notify = SIGEV_SIGNAL;
		sev.sigev_notify = SIGEV_THREAD_ID;
//		sev.sigev_notify_thread_id = syscall(SYS_gettid);
		sev._sigev_un._tid = syscall(SYS_gettid);
		sev.sigev_signo = SIG;
		sev.sigev_value.sival_ptr = &evm_global_timerid;
		if (timer_create(CLOCKID, &sev, &evm_global_timerid) == -1) {
			evm_log_return_system_err("timer_create() timer ID=%p\n", (void *)evm_global_timerid);
			pthread_mutex_unlock(&evm_global_timer_mutex);
		}

		evm_log_debug("Internal timers ID is %p\n", (void *)evm_global_timerid);

		evm_log_debug("Unblocking signal %d\n", SIG);
		if (pthread_sigmask(SIG_UNBLOCK, &sigmask, NULL) < 0) {
			evm_log_return_system_err("pthread_sigmask() SIG_UNBLOCK\n");
			pthread_mutex_unlock(&evm_global_timer_mutex);
		}
		evm_global_timer_not_created = 0;
	}
	pthread_mutex_unlock(&evm_global_timer_mutex);

	return 0;
}

static int timer_enqueue(evm_init_struct *evm_init_ptr, evm_timer_struct *tmr)
{
	int status = -1;
	evm_timer_queue_struct *tmr_queue = (evm_timer_queue_struct *)evm_init_ptr->tmr_queue;
	sem_t *bsem = &evm_init_ptr->blocking_sem;
	pthread_mutex_t *amtx = &tmr_queue->access_mutex;

	evm_log_info("(entry)\n");
	pthread_mutex_lock(amtx);
	if (tmr_queue->last_tmr == NULL)
		tmr_queue->first_tmr = tmr;
	else
		tmr_queue->last_tmr->next = tmr;

	tmr_queue->last_tmr = tmr;
	tmr->next = NULL;
	pthread_mutex_unlock(amtx);
	evm_log_info("Post blocking semaphore (UNBLOCK)\n");
	sem_post(bsem);
	return 0;
}

static evm_timer_struct * timer_dequeue(evm_init_struct *evm_init_ptr)
{
	evm_timer_struct *tmr;
	evm_timer_queue_struct *tmr_queue = (evm_timer_queue_struct *)evm_init_ptr->tmr_queue;
	pthread_mutex_t *amtx = &tmr_queue->access_mutex;

	evm_log_info("(entry)\n");
	pthread_mutex_lock(amtx);
	tmr = tmr_queue->first_tmr;
	if (tmr == NULL) {
		pthread_mutex_unlock(amtx);
		return NULL;
	}

	if (tmr->next == NULL) {
		tmr_queue->first_tmr = NULL;
		tmr_queue->last_tmr = NULL;
	} else
		tmr_queue->first_tmr = tmr->next;

	tmr->evm_ptr = evm_init_ptr; /*just in case:)*/
	pthread_mutex_unlock(amtx);
	return tmr;
}

static int evm_timer_pass(evm_init_struct *evm_init_ptr, evm_timer_struct *tmr)
{
	evm_log_info("(entry) evm_init_ptr=%p, msg=%p\n", evm_init_ptr, tmr);
	if (evm_init_ptr != NULL) {
		if (timer_enqueue(evm_init_ptr, tmr) != 0) {
			evm_log_error("Timer enqueuing failed!\n");
			return -1;
		}
		return 0;
	}
	return -1;
}

evm_timer_struct * evm_timers_check(evm_init_struct *evm_init_ptr)
{
	int semVal;
	evm_timer_struct *tmr_return = NULL;
	evm_timer_struct *tmr;
	struct timespec time_stamp;
	struct itimerspec its;
	int *thr_overruns;

	evm_log_info("(entry) evm_init_ptr=%p,  first_tmr=%p\n", evm_init_ptr, ((evm_timer_queue_struct *)evm_init_ptr->tmr_queue)->first_tmr);

	tmr = ((evm_timer_queue_struct *)evm_init_ptr->tmr_queue)->first_tmr;
	if (tmr != NULL)
		return timer_dequeue(evm_init_ptr); /*first consume already prepared expired timers*/

	/*Check the global timers queue for the newly expired timers.*/
	pthread_mutex_lock(&evm_global_timer_mutex);
	tmr = evm_global_tmr_queue.first_tmr;
	if (tmr == NULL) {
		pthread_mutex_unlock(&evm_global_timer_mutex);
		return NULL; /*no timers set*/
	}

	thr_overruns = (int *)pthread_getspecific(evm_thr_overruns_key);
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
		/* on clock_gettime() failure reschedule evm_timers_check() to next second */
		its.it_value.tv_sec = 1;
		if (timer_settime(evm_global_timerid, 0, &its, NULL) == -1) {
			evm_log_system_error("timer_settime timer ID=%p\n", (void *)evm_global_timerid);
		}
		pthread_mutex_unlock(&evm_global_timer_mutex);
		return NULL;
	}

	if (
		(time_stamp.tv_sec > tmr->tm_stamp.tv_sec) ||
		(
			(time_stamp.tv_sec == tmr->tm_stamp.tv_sec) &&
			(time_stamp.tv_nsec >= tmr->tm_stamp.tv_nsec)
		)
	) {
		evm_global_tmr_queue.first_tmr = tmr->next;
//		evm_log_debug("timer expired: tmr=%p, stopped=%d\n", (void *)tmr, tmr->stopped);

		tmr_return = tmr;
		tmr = evm_global_tmr_queue.first_tmr;
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
			if (timer_settime(evm_global_timerid, 0, &its, NULL) == -1) {
				evm_log_system_error("timer_settime timer ID=%p\n", (void *)evm_global_timerid);
			}
		} else {
			evm_log_debug("its(sec)=%ld, its(nsec)=%ld, tmr_return=%p\n", its.it_value.tv_sec, its.it_value.tv_nsec, tmr_return);
//			abort();
		}
	}
	pthread_mutex_unlock(&evm_global_timer_mutex);

	if (tmr_return != NULL) {
		if (tmr_return->evm_ptr == evm_init_ptr)
			return tmr_return;

		if (evm_timer_pass(tmr_return->evm_ptr, tmr_return) < 0) {
			evm_log_error("Timer passing failed!\n");
		}
	}

	return NULL;
}

evm_timer_struct * evm_timer_start(evm_init_struct *evm_init_ptr, evm_evids_list_struct *tmrid_ptr, time_t tv_sec, long tv_nsec, void *ctx_ptr)
{
	struct itimerspec its;
	evm_timer_struct *new, *prev, *tmr;

	evm_log_info("(entry)\n");
	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
		return NULL;
	}

	if (tmrid_ptr == NULL) {
		evm_log_error("Event machine timer undefined!\n");
		return NULL;
	}

	new = (evm_timer_struct *)calloc(1, sizeof(evm_timer_struct));
	if (new == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	evm_log_debug("New timer ptr=%p\n", (void *)new);

	new->evm_ptr = evm_init_ptr;
	new->tmrid_ptr = tmrid_ptr;
	new->saved = 0;
	new->stopped = 0;
	new->ctx_ptr = ctx_ptr;

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
	pthread_mutex_lock(&evm_global_timer_mutex);
	tmr = evm_global_tmr_queue.first_tmr;
	if (tmr == NULL) {
		if (timer_settime(evm_global_timerid, 0, &its, NULL) == -1) {
			evm_log_system_error("timer_settime timer ID=%p\n", (void *)evm_global_timerid);
			free(new);
			new = NULL;
			pthread_mutex_unlock(&evm_global_timer_mutex);
			return NULL;
		}

		evm_global_tmr_queue.first_tmr = new;
		pthread_mutex_unlock(&evm_global_timer_mutex);
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
			if (tmr == evm_global_tmr_queue.first_tmr) {
				if (timer_settime(evm_global_timerid, 0, &its, NULL) == -1) {
					evm_log_system_error("timer_settime timer ID=%p\n", (void *)evm_global_timerid);
					free(new);
					new = NULL;
					pthread_mutex_unlock(&evm_global_timer_mutex);
					return NULL;
				}
				evm_global_tmr_queue.first_tmr = new;
			} else
				prev->next = new;

			pthread_mutex_unlock(&evm_global_timer_mutex);
			return new;
		}

		prev = tmr;
		tmr = tmr->next;
	}

	/* The last in the list! */
	prev->next = new;
	pthread_mutex_unlock(&evm_global_timer_mutex);

	return new;
}

int evm_timer_stop(evm_timer_struct *timer)
{
	evm_log_info("(entry)\n");
	if (timer == NULL)
		return -1;

	timer->stopped = 1;

	return 0;
}

int evm_timer_finalize(void *ptr)
{
	evm_timer_struct *timer = (evm_timer_struct *)ptr;

	evm_log_info("(cb entry) ptr=%p\n", ptr);
	if (timer == NULL)
		return -1;

	if (timer->saved == 0) {
		free(timer);
	}

	return 0;
}

evm_evids_list_struct * evm_tmrid_add(evm_init_struct *evm_init_ptr, int tmr_id)
{
	evm_evids_list_struct *new, *tmp;
	evm_log_info("(entry)\n");

	if (evm_init_ptr == NULL)
		return NULL;

	tmp = evm_init_ptr->tmrids_first;
	while (tmp != NULL) {
		if (tmr_id == tmp->ev_id)
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
	new->evm_ptr = evm_init_ptr;
	new->evtype_ptr = NULL;
	new->ev_id = tmr_id;
	new->ev_prepare = NULL;
	new->ev_handle = NULL;
	new->ev_finalize = evm_timer_finalize;
	new->prev = tmp;
	new->next = NULL;
	if (tmp != NULL)
		tmp->next = new;
	else
		evm_init_ptr->tmrids_first = new;
	return new;
}

evm_evids_list_struct * evm_tmrid_get(evm_init_struct *evm_init_ptr, int tmr_id)
{
	evm_evids_list_struct *tmp = NULL;
	evm_log_info("(entry)\n");

	if (evm_init_ptr == NULL)
		return NULL;

	tmp = evm_init_ptr->tmrids_first;
	while (tmp != NULL) {
		evm_log_debug("evm_tmrid_get() tmp=%p,tmp->ev_id=%d, tmr_id=%d\n", (void *)tmp, tmp->ev_id, tmr_id);
		if (tmr_id == tmp->ev_id)
			return tmp;
		tmp = tmp->next;
	}

	return NULL;
}

evm_evids_list_struct * evm_tmrid_del(evm_init_struct *evm_init_ptr, int tmr_id)
{
	evm_evids_list_struct *tmp = NULL;
	evm_log_info("(entry)\n");

	if (evm_init_ptr == NULL)
		return NULL;

	tmp = evm_init_ptr->tmrids_first;
	while (tmp != NULL) {
		if (tmr_id == tmp->ev_id)
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

int evm_tmrid_handle_cb_set(evm_evids_list_struct *tmrid_ptr, int (*ev_handle)(void *ev_ptr))
{
	int rv = 0;
	evm_log_info("(entry)\n");

	if (tmrid_ptr == NULL)
		return -1;

	if (ev_handle == NULL)
		return -1;

	if (rv == 0) {
		tmrid_ptr->ev_handle = ev_handle;
	}
	return rv;
}

int evm_tmrid_finalize_cb_set(evm_evids_list_struct *tmrid_ptr, int (*ev_finalize)(void *ev_ptr))
{
	int rv = 0;
	evm_log_info("(entry)\n");

	if (tmrid_ptr == NULL)
		return -1;

	if (ev_finalize == NULL)
		return -1;

	if (rv == 0) {
		tmrid_ptr->ev_finalize = ev_finalize;
	}
	return rv;
}

