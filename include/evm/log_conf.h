/*
 * log_conf.h
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

extern unsigned int use_syslog;
extern unsigned int log_normal;
extern unsigned int log_verbose;
extern unsigned int log_trace;
extern unsigned int log_debug;

#define LOG_MODULE_INIT(name, value)\
static const char *log_module_name = #name;\
static const unsigned int log_module_value = value;\

#define _5DIGIT_SECS(timespec_x) (timespec_x.tv_sec % 100000)
#define _6DIGIT_USECS(timespec_x) (timespec_x.tv_nsec / 1000)

#define EVMLOG_DEBUG_FORMAT "[%05ld.%06ld|%d|%s|%s:%d] %s(): "
#define EVMLOG_DEBUG_ARGS _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), log_module_name, __FILE__, __LINE__, __FUNCTION__
#define EVMLOG_TRACE_FORMAT "[%05ld.%06ld|%d|%s] %s(): "
#define EVMLOG_TRACE_ARGS _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), log_module_name, __FUNCTION__
#define EVMLOG_NORMAL_FORMAT "[%05ld.%06ld|%d|%s] "
#define EVMLOG_NORMAL_ARGS _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), log_module_name

#define log_system_error(format, args...) {\
	struct timespec ts;\
	char buf[1024];\
	strerror_r(errno, buf, 1024);\
	clock_gettime(CLOCK_MONOTONIC, &ts);\
	if (use_syslog) {\
		if (LOG_MODULE_DEBUG && log_debug) {\
			syslog(LOG_ERR, EVMLOG_DEBUG_FORMAT "%s >> " format, EVMLOG_DEBUG_ARGS, buf, ##args);\
		} else if (LOG_MODULE_TRACE && log_trace) {\
			syslog(LOG_ERR, EVMLOG_TRACE_FORMAT "%s >> " format, EVMLOG_TRACE_ARGS, buf, ##args);\
		} else {\
			syslog(LOG_ERR, EVMLOG_NORMAL_FORMAT "%s >> " format, EVMLOG_NORMAL_ARGS, buf, ##args);\
		}\
	} else {\
		if (LOG_MODULE_DEBUG && log_debug) {\
			fprintf(stderr, EVMLOG_DEBUG_FORMAT "%s >> " format, EVMLOG_DEBUG_ARGS, buf, ##args);\
		} else if (LOG_MODULE_TRACE && log_trace) {\
			fprintf(stderr, EVMLOG_TRACE_FORMAT "%s >> " format, EVMLOG_TRACE_ARGS, buf, ##args);\
		} else {\
			fprintf(stderr, EVMLOG_NORMAL_FORMAT "%s >> " format, EVMLOG_NORMAL_ARGS, buf, ##args);\
		}\
		fflush(stderr);\
	}\
}

#define log_error(format, args...) {\
	struct timespec ts;\
	clock_gettime(CLOCK_MONOTONIC, &ts);\
	if (use_syslog) {\
		if (LOG_MODULE_DEBUG && log_debug) {\
			syslog(LOG_ERR, EVMLOG_DEBUG_FORMAT format, EVMLOG_DEBUG_ARGS, ##args);\
		} else if (LOG_MODULE_TRACE && log_trace) {\
			syslog(LOG_ERR, EVMLOG_TRACE_FORMAT format, EVMLOG_TRACE_ARGS, ##args);\
		} else {\
			syslog(LOG_ERR, EVMLOG_NORMAL_FORMAT format, EVMLOG_NORMAL_ARGS, ##args);\
		}\
	} else {\
		if (LOG_MODULE_DEBUG && log_debug) {\
			fprintf(stderr, EVMLOG_DEBUG_FORMAT format, EVMLOG_DEBUG_ARGS, ##args);\
		} else if (LOG_MODULE_TRACE && log_trace) {\
			fprintf(stderr, EVMLOG_TRACE_FORMAT format, EVMLOG_TRACE_ARGS, ##args);\
		} else {\
			fprintf(stderr, EVMLOG_NORMAL_FORMAT format, EVMLOG_NORMAL_ARGS, ##args);\
		}\
		fflush(stderr);\
	}\
}

#define log_warning(format, args...) {\
	struct timespec ts;\
	clock_gettime(CLOCK_MONOTONIC, &ts);\
	if (use_syslog) {\
		if (LOG_MODULE_DEBUG && log_debug) {\
			syslog(LOG_WARNING, EVMLOG_DEBUG_FORMAT format, EVMLOG_DEBUG_ARGS, ##args);\
		} else if (LOG_MODULE_TRACE && log_trace) {\
			syslog(LOG_WARNING, EVMLOG_TRACE_FORMAT format, EVMLOG_TRACE_ARGS, ##args);\
		} else if (log_normal || log_verbose) {\
			syslog(LOG_WARNING, EVMLOG_NORMAL_FORMAT format, EVMLOG_NORMAL_ARGS, ##args);\
		}\
	} else {\
		if (LOG_MODULE_DEBUG && log_debug) {\
			fprintf(stdout, EVMLOG_DEBUG_FORMAT format, EVMLOG_DEBUG_ARGS, ##args);\
			fflush(stdout);\
		} else if (LOG_MODULE_TRACE && log_trace) {\
			fprintf(stdout, EVMLOG_TRACE_FORMAT format, EVMLOG_TRACE_ARGS, ##args);\
			fflush(stdout);\
		} else if (log_normal || log_verbose) {\
			fprintf(stdout, EVMLOG_NORMAL_FORMAT format, EVMLOG_NORMAL_ARGS, ##args);\
			fflush(stdout);\
		}\
	}\
}

#define log_notice(format, args...) {\
	struct timespec ts;\
	clock_gettime(CLOCK_MONOTONIC, &ts);\
	if (use_syslog) {\
		if (LOG_MODULE_DEBUG && log_debug) {\
			syslog(LOG_NOTICE, EVMLOG_DEBUG_FORMAT format, EVMLOG_DEBUG_ARGS, ##args);\
		} else if (LOG_MODULE_TRACE && log_trace) {\
			syslog(LOG_NOTICE, EVMLOG_TRACE_FORMAT format, EVMLOG_TRACE_ARGS, ##args);\
		} else if (log_normal || log_verbose) {\
			syslog(LOG_NOTICE, EVMLOG_NORMAL_FORMAT format, EVMLOG_NORMAL_ARGS, ##args);\
		}\
	} else {\
		if (LOG_MODULE_DEBUG && log_debug) {\
			fprintf(stdout, EVMLOG_DEBUG_FORMAT format, EVMLOG_DEBUG_ARGS, ##args);\
			fflush(stdout);\
		} else if (LOG_MODULE_TRACE && log_trace) {\
			fprintf(stdout, EVMLOG_TRACE_FORMAT format, EVMLOG_TRACE_ARGS, ##args);\
			fflush(stdout);\
		} else if (log_normal || log_verbose) {\
			fprintf(stdout, EVMLOG_NORMAL_FORMAT format, EVMLOG_NORMAL_ARGS, ##args);\
			fflush(stdout);\
		}\
	}\
}

#define log_info(format, args...) {\
	struct timespec ts;\
	clock_gettime(CLOCK_MONOTONIC, &ts);\
	if (use_syslog) {\
		if (LOG_MODULE_DEBUG && log_debug) {\
			syslog(LOG_INFO, EVMLOG_DEBUG_FORMAT format, EVMLOG_DEBUG_ARGS, ##args);\
		} else if (LOG_MODULE_TRACE && log_trace) {\
			syslog(LOG_INFO, EVMLOG_TRACE_FORMAT format, EVMLOG_TRACE_ARGS, ##args);\
		} else if (log_verbose) {\
			syslog(LOG_INFO, EVMLOG_NORMAL_FORMAT format, EVMLOG_NORMAL_ARGS, ##args);\
		}\
	} else {\
		if (LOG_MODULE_DEBUG && log_debug) {\
			fprintf(stdout, EVMLOG_DEBUG_FORMAT format, EVMLOG_DEBUG_ARGS, ##args);\
			fflush(stdout);\
		} else if (LOG_MODULE_TRACE && log_trace) {\
			fprintf(stdout, EVMLOG_TRACE_FORMAT format, EVMLOG_TRACE_ARGS, ##args);\
			fflush(stdout);\
		} else if (log_verbose) {\
			fprintf(stdout, EVMLOG_NORMAL_FORMAT format, EVMLOG_NORMAL_ARGS, ##args);\
			fflush(stdout);\
		}\
	}\
}

#define log_debug(format, args...) {\
	struct timespec ts;\
	clock_gettime(CLOCK_MONOTONIC, &ts);\
	if (use_syslog) {\
		if (LOG_MODULE_DEBUG && log_debug) {\
			syslog(LOG_DEBUG, EVMLOG_DEBUG_FORMAT format, EVMLOG_DEBUG_ARGS, ##args);\
		}\
	} else {\
		if (LOG_MODULE_DEBUG && log_debug) {\
			fprintf(stdout, EVMLOG_DEBUG_FORMAT format, EVMLOG_DEBUG_ARGS, ##args);\
			fflush(stdout);\
		}\
	}\
}

#define return_system_err(msg, args...) {\
	int errsv = errno;\
	log_system_error(msg, ##args);\
	return -errsv;\
}

#define return_err(msg, args...) {\
	int errsv = errno;\
	log_error(msg, ##args);\
	return -errsv;\
}

#endif /*log_conf_h*/
