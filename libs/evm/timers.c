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

#ifndef evm_timers_c
#define evm_timers_c
#else
#error Preprocesor macro evm_timers_c conflict!
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

typedef struct timer_queue timer_queue_struct;

struct timer_queue {
	evm_timer_struct *first_tmr;
	evm_timer_struct *last_tmr;
	evm_fd_struct *evmfd; /*internal timer queue FD binding - eventfd()*/
	pthread_mutex_t mutex;
};

static int timer_queue_evmfd_read(int efd, evm_message_struct *ptr);

/* global timer ID */
timer_t global_timerid;
timer_queue_struct global_tmr_queue;
static int global_timer_not_created = 1;
static pthread_mutex_t global_timer_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Thread-specific data related. */
static int thr_keys_not_created = 1;
static pthread_key_t thr_overruns_key; /*initialized only once - not for each call to evm_init() within a process*/

static void timers_sighandler(int signum, siginfo_t *siginfo, void *context)
{
	if (signum == SIG) {
		int *thr_overruns = (int *)pthread_getspecific(thr_overruns_key);
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
	if (thr_keys_not_created) {
		if ((errno = pthread_key_create(&thr_overruns_key, NULL)) != 0) {
			evm_log_return_system_err("pthread_key_create()\n");
		}
		thr_keys_not_created = 0;
	}
	if ((ptr = calloc(1, sizeof(int))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): thread-specific data\n");
		return status;
	}
	if ((errno = pthread_setspecific(thr_overruns_key, ptr)) != 0) {
		evm_log_return_system_err("pthread_setspecific()\n");
	}
	*(int *)ptr = -2; /*initialize*/

	/* Setup thread specific internal timer queue. */
	if ((ptr = calloc(1, sizeof(timer_queue_struct))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): internal timer queue\n");
		return status;
	}
	evm_init_ptr->tmr_queue = (timer_queue_struct *)ptr;
	((timer_queue_struct *)evm_init_ptr->tmr_queue)->first_tmr = NULL;
	((timer_queue_struct *)evm_init_ptr->tmr_queue)->last_tmr = NULL;
	pthread_mutex_init(&((timer_queue_struct *)ptr)->mutex, NULL);
	pthread_mutex_unlock(&((timer_queue_struct *)ptr)->mutex);

	pthread_mutex_lock(&global_timer_mutex);
	if (global_timer_not_created) {
		/* Block timer signal temporarily */
		evm_log_debug("Blocking signal %d\n", SIG);
		sigemptyset(&sigmask);
		sigaddset(&sigmask, SIG);
		if (pthread_sigmask(SIG_BLOCK, &sigmask, NULL) < 0) {
			evm_log_return_system_err("pthread_sigmask() SIG_BLOCK\n");
			pthread_mutex_unlock(&global_timer_mutex);
		}

		/* Establish handler for timer signal */
		evm_log_debug("Establishing handler for signal %d\n", SIG);
		sact.sa_flags = SA_SIGINFO;
		sact.sa_sigaction = timers_sighandler;
		sigemptyset(&sact.sa_mask);
		if (sigaction(SIG, &sact, NULL) == -1) {
			evm_log_return_system_err("sigaction()\n");
			pthread_mutex_unlock(&global_timer_mutex);
		}

		/* Create the timer */
#if 0 /*orig*/
		if ((evm_init_ptr->timerid = (timer_t *)calloc(1, sizeof(timer_t))) == NULL) {
			errno = ENOMEM;
			evm_log_system_error("calloc(): internal timer ID\n");
			pthread_mutex_unlock(&global_timer_mutex);
			return status;
		}
#endif
//		sev.sigev_notify = SIGEV_SIGNAL;
		sev.sigev_notify = SIGEV_THREAD_ID;
//		sev.sigev_notify_thread_id = syscall(SYS_gettid);
		sev._sigev_un._tid = syscall(SYS_gettid);
		sev.sigev_signo = SIG;
		sev.sigev_value.sival_ptr = &global_timerid;
		if (timer_create(CLOCKID, &sev, &global_timerid) == -1) {
			evm_log_return_system_err("timer_create() timer ID=%p\n", (void *)global_timerid);
			pthread_mutex_unlock(&global_timer_mutex);
		}

		evm_log_debug("Internal timers ID is %p\n", (void *)global_timerid);

		evm_log_debug("Unblocking signal %d\n", SIG);
		if (pthread_sigmask(SIG_UNBLOCK, &sigmask, NULL) < 0) {
			evm_log_return_system_err("pthread_sigmask() SIG_UNBLOCK\n");
			pthread_mutex_unlock(&global_timer_mutex);
		}
		global_timer_not_created = 0;
	}
	pthread_mutex_unlock(&global_timer_mutex);

	return 0;
}

int evm_timers_queue_fd_init(evm_init_struct *evm_init_ptr)
{
	int status = -1;
//	void *ptr;
	evm_fd_struct *ptr;

	evm_log_info("(entry) evm_init_ptr=%p\n", evm_init_ptr);
	/* Prepare internal timer queue FD for EPOLL to operate over internal linked list. */
	if ((ptr = (evm_fd_struct *)calloc(1, sizeof(evm_fd_struct))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): internal timer queue evm FD\n");
		return status;
	}
	((timer_queue_struct *)evm_init_ptr->tmr_queue)->evmfd = ptr;

	if ((ptr->fd = eventfd(0, EFD_CLOEXEC)) < 0)
		evm_log_return_system_err("eventfd()\n");

	ptr->ev_epoll.events = EPOLLIN;
	ptr->ev_epoll.data.ptr = ptr; /*our own address*/
	ptr->msg_receive = timer_queue_evmfd_read;

	evm_log_debug("ptr: %p, &ptr->ev_epoll: %p\n", ptr, &ptr->ev_epoll);
	evm_log_debug("evm_init_ptr->epollfd: %d, ptr->fd: %d\n", evm_init_ptr->epollfd, ptr->fd);
	if (epoll_ctl(evm_init_ptr->epollfd, EPOLL_CTL_ADD, ptr->fd, &ptr->ev_epoll) < 0) {
		evm_log_return_system_err("epoll_ctl()\n");
	}

	return 0;
}

static int timer_enqueue(evm_init_struct *evm_init_ptr, evm_timer_struct *tmr)
{
	int status = -1;
	timer_queue_struct *tmr_queue = (timer_queue_struct *)evm_init_ptr->tmr_queue;
	pthread_mutex_t *mtx = &tmr_queue->mutex;

	evm_log_info("(entry)\n");
	pthread_mutex_lock(mtx);
	if (tmr_queue->last_tmr == NULL)
		tmr_queue->first_tmr = tmr;
	else
		tmr_queue->last_tmr->next = tmr;

	tmr_queue->last_tmr = tmr;
	tmr->next = NULL;
	pthread_mutex_unlock(mtx);
	return 0;
}

static evm_timer_struct * timer_dequeue(evm_init_struct *evm_init_ptr)
{
	evm_timer_struct *tmr;
	timer_queue_struct *tmr_queue = (timer_queue_struct *)evm_init_ptr->tmr_queue;
	pthread_mutex_t *mtx = &tmr_queue->mutex;

	evm_log_info("(entry)\n");
	pthread_mutex_lock(mtx);
	tmr = tmr_queue->first_tmr;
	if (tmr == NULL) {
		pthread_mutex_unlock(mtx);
		return NULL;
	}

	if (tmr->next == NULL) {
		tmr_queue->first_tmr = NULL;
		tmr_queue->last_tmr = NULL;
	} else
		tmr_queue->first_tmr = tmr->next;

	tmr->evm_ptr = evm_init_ptr; /*just in case:)*/
	pthread_mutex_unlock(mtx);
	return tmr;
}

static int timer_queue_evmfd_read(int efd, evm_message_struct *ptr)
{
	uint64_t u;
	ssize_t s;

	evm_log_info("(cb entry) eventfd=%d\n", efd);
	ptr = NULL;
	if ((s = read(efd, &u, sizeof(uint64_t))) != sizeof(uint64_t))
		evm_log_system_error("read(): timer_queue_evmfd_read\n");

	return 0;
}

static int evm_timer_pass(evm_init_struct *evm_init_ptr, evm_timer_struct *tmr)
{
	uint64_t u;
	ssize_t s;

	evm_log_info("(entry) evm_init_ptr=%p, tmr=%p\n", evm_init_ptr, tmr);
	if (evm_init_ptr != NULL) {
		if (timer_enqueue(evm_init_ptr, tmr) != 0) {
			evm_log_error("Timer enqueuing failed!\n");
			return -1;
		}

		u = 1;
		if ((s = write(((timer_queue_struct *)evm_init_ptr->tmr_queue)->evmfd->fd, &u, sizeof(uint64_t))) != sizeof(uint64_t)) {
			evm_log_system_error("write(): evm_timer_pass\n");
			if (timer_dequeue(evm_init_ptr) == NULL) {
				evm_log_error("Timer dequeuing failed!\n");
			}
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

	evm_log_info("(entry) evm_init_ptr=%p,  first_tmr=%p\n", evm_init_ptr, ((timer_queue_struct *)evm_init_ptr->tmr_queue)->first_tmr);

	tmr = ((timer_queue_struct *)evm_init_ptr->tmr_queue)->first_tmr;
	if (tmr != NULL)
		return timer_dequeue(evm_init_ptr); /*first consume already prepared expired timers*/

	/*Check the global timers queue for the newly expired timers.*/
	pthread_mutex_lock(&global_timer_mutex);
	tmr = global_tmr_queue.first_tmr;
	if (tmr == NULL) {
		pthread_mutex_unlock(&global_timer_mutex);
		return NULL; /*no timers set*/
	}

	thr_overruns = (int *)pthread_getspecific(thr_overruns_key);
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
		/* on clock_gettime() failure reschedule timer_check() to next second */
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
		global_tmr_queue.first_tmr = tmr->next;
//		evm_log_debug("timer expired: tmr=%p, stopped=%d\n", (void *)tmr, tmr->stopped);

		tmr_return = tmr;
		tmr = global_tmr_queue.first_tmr;
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
		if (tmr_return->evm_ptr == evm_init_ptr)
			return tmr_return;

		if (evm_timer_pass(tmr_return->evm_ptr, tmr_return) < 0) {
			evm_log_error("Timer passing failed!\n");
		}
	}

	return NULL;
}

evm_timer_struct * evm_timer_start(evm_init_struct *evm_init_ptr, evm_ids_struct tmr_evm_ids, time_t tv_sec, long tv_nsec, void *ctx_ptr)
{
	struct itimerspec its;
	evm_timer_struct *new, *prev, *tmr;

	evm_log_info("(entry)\n");
	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
		return NULL;
	}

	new = (evm_timer_struct *)calloc(1, sizeof(evm_timer_struct));
	if (new == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	evm_log_debug("New timer ptr=%p\n", (void *)new);

	new->evm_ptr = evm_init_ptr;
	new->saved = 0;
	new->stopped = 0;
	new->ctx_ptr = ctx_ptr;
	new->tmr_ids.ev_id = tmr_evm_ids.ev_id;
	new->tmr_ids.evm_idx = tmr_evm_ids.evm_idx;

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
	tmr = global_tmr_queue.first_tmr;
	if (tmr == NULL) {
		if (timer_settime(global_timerid, 0, &its, NULL) == -1) {
			evm_log_system_error("timer_settime timer ID=%p\n", (void *)global_timerid);
			free(new);
			new = NULL;
			pthread_mutex_unlock(&global_timer_mutex);
			return NULL;
		}

		global_tmr_queue.first_tmr = new;
		pthread_mutex_unlock(&global_timer_mutex);
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
			if (tmr == global_tmr_queue.first_tmr) {
				if (timer_settime(global_timerid, 0, &its, NULL) == -1) {
					evm_log_system_error("timer_settime timer ID=%p\n", (void *)global_timerid);
					free(new);
					new = NULL;
					pthread_mutex_unlock(&global_timer_mutex);
					return NULL;
				}
				global_tmr_queue.first_tmr = new;
			} else
				prev->next = new;

			pthread_mutex_unlock(&global_timer_mutex);
			return new;
		}

		prev = tmr;
		tmr = tmr->next;
	}

	/* The last in the list! */
	prev->next = new;
	pthread_mutex_unlock(&global_timer_mutex);

	return new;
}

int evm_timer_stop(evm_timer_struct *timer)
{
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

