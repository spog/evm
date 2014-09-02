/*
 * printf_conf.h
 *
 *  Copyright (C) 2012  Samo Pogacnik
 */

/*
 * This is various printf output definition header file.
 */

#ifndef log_conf_h
#define log_conf_h

/*
 * Here starts the PUBLIC stuff.
 */

#include <errno.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <time.h>
#include <sys/syscall.h>
#include <sys/types.h>

extern unsigned int log_mask;
extern unsigned int use_syslog;

#define _5DIGIT_SECS(timespec_x) (timespec_x.tv_sec % 100000)
#define _6DIGIT_USECS(timespec_x) (timespec_x.tv_nsec / 1000)

#define log_error(format, args...) {\
	struct timespec ts;\
	char buf[1024];\
	strerror_r(errno, buf, 1024);\
	clock_gettime(CLOCK_MONOTONIC, &ts);\
	if (use_syslog) {\
		if (LOG_MASK(LOG_DEBUG) & log_mask) {\
			syslog(LOG_ERR, "[%05ld.%06ld] [%d] %s @ %d: %s() - %s: " format, _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), __FILE__, __LINE__, __FUNCTION__, buf, ##args);\
		} else {\
			syslog(LOG_ERR, "[%05ld.%06ld] [%d] - %s: " format, _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), buf, ##args);\
		}\
	} else {\
		if (LOG_MASK(LOG_DEBUG) & log_mask) {\
			fprintf(stderr, "[%05ld.%06ld] [%d] %s @ %d: %s() - %s: " format, _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), __FILE__, __LINE__, __FUNCTION__, buf, ##args);\
		} else {\
			fprintf(stderr, "[%05ld.%06ld] [%d] - %s: " format, _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), buf, ##args);\
		}\
		fflush(stderr);\
	}\
}

#define log_verbose(format, args...) {\
	if (COMP_MODULE) {\
		struct timespec ts;\
		clock_gettime(CLOCK_MONOTONIC, &ts);\
		if (use_syslog) {\
			if (LOG_MASK(LOG_DEBUG) & log_mask) {\
				syslog(LOG_ERR, "[%05ld.%06ld] [%d] %s @ %d: %s(): " format, _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), __FILE__, __LINE__, __FUNCTION__, ##args);\
			} else {\
				syslog(LOG_WARNING, "[%05ld.%06ld] [%d]: " format, _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), ##args);\
			}\
		} else {\
			if (LOG_MASK(LOG_DEBUG) & log_mask) {\
				fprintf(stdout, "[%05ld.%06ld] [%d] %s @ %d: %s(): " format, _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), __FILE__, __LINE__, __FUNCTION__, ##args);\
			} else {\
				fprintf(stdout, "[%05ld.%06ld] [%d]: " format, _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), ##args);\
			}\
			fflush(stdout);\
		}\
	}\
}

#define log_normal(format, args...) {\
	if (COMP_MODULE) {\
		struct timespec ts;\
		clock_gettime(CLOCK_MONOTONIC, &ts);\
		if (use_syslog) {\
			if (LOG_MASK(LOG_DEBUG) & log_mask) {\
				syslog(LOG_ERR, "[%05ld.%06ld] [%d] %s @ %d: %s(): " format, _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), __FILE__, __LINE__, __FUNCTION__, ##args);\
			} else {\
				syslog(LOG_NOTICE, "[%05ld.%06ld] [%d]: " format, _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), ##args);\
			}\
		} else {\
			if (LOG_MASK(LOG_DEBUG) & log_mask) {\
				fprintf(stdout, "[%05ld.%06ld] [%d] %s @ %d: %s(): " format, _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), __FILE__, __LINE__, __FUNCTION__, ##args);\
			} else {\
				fprintf(stdout, "[%05ld.%06ld] [%d]: " format, _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), ##args);\
			}\
			fflush(stdout);\
		}\
	}\
}

#define log_trace(format, args...) {\
	if (COMP_MODULE) {\
		struct timespec ts;\
		clock_gettime(CLOCK_MONOTONIC, &ts);\
		if (use_syslog) {\
			if (LOG_MASK(LOG_DEBUG) & log_mask) {\
				syslog(LOG_ERR, "[%05ld.%06ld] [%d] %s @ %d: %s(): " format, _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), __FILE__, __LINE__, __FUNCTION__, ##args);\
			} else {\
				syslog(LOG_INFO, "[%05ld.%06ld] [%d]: " format, _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), ##args);\
			}\
		} else {\
			if (LOG_MASK(LOG_DEBUG) & log_mask) {\
				fprintf(stdout, "[%05ld.%06ld] [%d] %s @ %d: %s(): " format, _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), __FILE__, __LINE__, __FUNCTION__, ##args);\
			} else {\
				fprintf(stdout, "[%05ld.%06ld] [%d]: " format, _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), ##args);\
			}\
			fflush(stdout);\
		}\
	}\
}

#define log_debug(format, args...) {\
	if (COMP_MODULE) {\
		struct timespec ts;\
		clock_gettime(CLOCK_MONOTONIC, &ts);\
		if (use_syslog) {\
			if (LOG_MASK(LOG_DEBUG) & log_mask) {\
				syslog(LOG_ERR, "[%05ld.%06ld] [%d] %s @ %d: %s(): " format, _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), __FILE__, __LINE__, __FUNCTION__, ##args);\
			}\
		} else {\
			if (LOG_MASK(LOG_DEBUG) & log_mask) {\
				fprintf(stdout, "[%05ld.%06ld] [%d] %s @ %d: %s(): " format, _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), __FILE__, __LINE__, __FUNCTION__, ##args);\
			}\
			fflush(stdout);\
		}\
	}\
}

#define return_err(msg, args...) {\
	int errsv = errno;\
	log_error(msg, ##args);\
	return -errsv;\
}

#endif
