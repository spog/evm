/*
 *  Copyright (C) 2012  Samo Pogacnik
 */

/*
 * The EVM timers module
 */
#ifndef evm_timers_c
#define evm_timers_c
#else
#error Preprocesor macro evm_timers_c conflict!
#endif

#include "timers.h"

static sem_t semaphore;

static timer_t timerid;

static void timers_sighandler(int signum, siginfo_t *siginfo, void *context)
{
	if (signum == SIG) {
		int overruns = timer_getoverrun(timerid);
		while (overruns > 0) {
			overruns--;
			sem_post(&semaphore);
		}
		sem_post(&semaphore);
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

	/* Setup internal message queue. */
	if ((ptr = calloc(1, sizeof(timer_queue_struct))) == NULL) {
		errno = ENOMEM;
		evm_log_system_error("calloc(): internal timer queue\n");
		return status;
	}
	evm_init_ptr->tmr_queue = (timer_queue_struct *)ptr;
	((timer_queue_struct *)evm_init_ptr->tmr_queue)->first_tmr = NULL;

	if (sem_init(&semaphore, 0, 0) == -1)
		evm_log_return_system_err("sem_init()\n");

	/* Block timer signal temporarily */
	evm_log_debug("Blocking signal %d\n", SIG);
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIG);
	if (pthread_sigmask(SIG_BLOCK, &sigmask, NULL) < 0) {
		evm_log_return_system_err("pthread_sigmask() SIG_BLOCK\n");
	}

	/* Establish handler for timer signal */
	evm_log_debug("Establishing handler for signal %d\n", SIG);
	sact.sa_flags = SA_SIGINFO;
	sact.sa_sigaction = timers_sighandler;
	sigemptyset(&sact.sa_mask);
	if (sigaction(SIG, &sact, NULL) == -1)
		evm_log_return_system_err("sigaction()\n");

	/* Create the timer */
//	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_notify = SIGEV_THREAD_ID;
//	sev.sigev_notify_thread_id = syscall(SYS_gettid);
	sev._sigev_un._tid = syscall(SYS_gettid);
	sev.sigev_signo = SIG;
	sev.sigev_value.sival_ptr = &timerid;
	if (timer_create(CLOCKID, &sev, &timerid) == -1)
		evm_log_return_system_err("timer_create() timerid=%lx\n", (unsigned long)timerid);

	evm_log_debug("Internal timers ID is 0x%lx\n", (unsigned long) timerid);

	evm_log_debug("Unblocking signal %d\n", SIG);
	if (pthread_sigmask(SIG_UNBLOCK, &sigmask, NULL) < 0) {
		evm_log_return_system_err("pthread_sigmask() SIG_UNBLOCK\n");
	}

	return 0;
}

evm_timer_struct * evm_timers_check(evm_init_struct *evm_init_ptr)
{
	int semVal;
	evm_timer_struct *tmr_return = NULL;
	evm_timer_struct *tmr;
	struct timespec time_stamp;
	struct itimerspec its;

	evm_log_info("(entry) first_tmr=%p\n", ((timer_queue_struct *)evm_init_ptr->tmr_queue)->first_tmr);

	tmr = ((timer_queue_struct *)evm_init_ptr->tmr_queue)->first_tmr;
	if (tmr == NULL)
		return NULL; /*no timers set*/

	sem_getvalue(&semaphore, &semVal); /*after signal, check semaphore*/

	if (semVal > 0) {
//		evm_log_debug("timers semaphore: semVal=%d\n", semVal);
		sem_trywait(&semaphore); /*after signal, lock semaphore*/
	}

	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;

	if (clock_gettime(CLOCK_MONOTONIC, &time_stamp) == -1) {
		evm_log_system_error("clock_gettime()\n");
		/* on clock_gettime() failure reschedule timer_check() to next second */
		its.it_value.tv_sec = 1;
		if (timer_settime(timerid, 0, &its, NULL) == -1) {
			evm_log_system_error("timer_settime timerid=%lx\n", (unsigned long)timerid);
		}
		return NULL;
	}

	if (
		(time_stamp.tv_sec > tmr->tm_stamp.tv_sec) ||
		(
			(time_stamp.tv_sec == tmr->tm_stamp.tv_sec) &&
			(time_stamp.tv_nsec >= tmr->tm_stamp.tv_nsec)
		)
	) {
		((timer_queue_struct *)evm_init_ptr->tmr_queue)->first_tmr = tmr->next_tmr;
//		evm_log_debug("timer expired: tmr=0x%x, stopped=%d\n", (unsigned int)tmr, tmr->stopped);

		tmr_return = tmr;
		tmr = ((timer_queue_struct *)evm_init_ptr->tmr_queue)->first_tmr;
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
			if (timer_settime(timerid, 0, &its, NULL) == -1) {
				evm_log_system_error("timer_settime timerid=%lx\n", (unsigned long)timerid);
			}
		} else {
			evm_log_debug("its(sec)=%ld, its(nsec)=%ld, tmr_return=%p\n", its.it_value.tv_sec, its.it_value.tv_nsec, tmr_return);
//			abort();
		}
	}

	if (tmr_return != NULL)
		tmr_return->evm_ptr = evm_init_ptr;

	return tmr_return;
}

evm_timer_struct * evm_timer_start(evm_init_struct *evm_init_ptr, evm_ids_struct tmr_evm_ids, time_t tv_sec, long tv_nsec, void *ctx_ptr)
{
	struct itimerspec its;
	evm_timer_struct *new, *prev, *tmr;
	evm_tab_struct *evm_tab;

	evm_log_info("(entry)\n");
	if (evm_init_ptr == NULL) {
		evm_log_error("Event machine init structure undefined!\n");
		return NULL;
	}

	evm_tab = evm_init_ptr->evm_tab;
	tmr = ((timer_queue_struct *)evm_init_ptr->tmr_queue)->first_tmr;

	new = (evm_timer_struct *)calloc(1, sizeof(evm_timer_struct));
	if (new == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	evm_log_debug("New timer tmr=0x%x\n", (unsigned int)new);

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
	if (tmr == NULL) {
		if (timer_settime(timerid, 0, &its, NULL) == -1) {
			evm_log_system_error("timer_settime timerid=%lx\n", (unsigned long)timerid);
			free(new);
			new = NULL;
			return NULL;
		}

		((timer_queue_struct *)evm_init_ptr->tmr_queue)->first_tmr = new;
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
			new->next_tmr = tmr;
			/* If first in the list, we are the first to expire -> start it!*/
			if (tmr == ((timer_queue_struct *)evm_init_ptr->tmr_queue)->first_tmr) {
				if (timer_settime(timerid, 0, &its, NULL) == -1) {
					evm_log_system_error("timer_settime timerid=%lx\n", (unsigned long)timerid);
					free(new);
					new = NULL;
					return NULL;
				}
				((timer_queue_struct *)evm_init_ptr->tmr_queue)->first_tmr = new;
			} else
				prev->next_tmr = new;

			return new;
		}

		prev = tmr;
		tmr = tmr->next_tmr;
	}

	/* The last in the list! */
	prev->next_tmr = new;

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

