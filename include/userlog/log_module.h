/*
 * The EVMLOG module
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

/*
 * This EVMLOG module provides various output definitions in a single header file.
 */

#ifndef EVM_FILE_log_module_h
#define EVM_FILE_log_module_h

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

extern unsigned int evmlog_use_syslog;
extern unsigned int evmlog_normal;
extern unsigned int evmlog_verbose;
extern unsigned int evmlog_trace;
extern unsigned int evmlog_debug;
extern unsigned int evmlog_add_header;

#define EVMLOG_MODULE_INIT(name, value)\
static const char *evmlog_module_name = #name;\
static const unsigned int evmlog_module_value = value;\

#define _5DIGIT_SECS(timespec_x) (timespec_x.tv_sec % 100000)
#define _6DIGIT_USECS(timespec_x) (timespec_x.tv_nsec / 1000)

#define EVMLOG_WITH_HEADER_DEBUG_FORMAT "[%05ld.%06ld|%d|%s] %s:%d %s(): "
#define EVMLOG_WITH_HEADER_DEBUG_ARGS _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), evmlog_module_name, __FILE__, __LINE__, __FUNCTION__
#define EVMLOG_WITH_HEADER_TRACE_FORMAT "[%05ld.%06ld|%d|%s] %s(): "
#define EVMLOG_WITH_HEADER_TRACE_ARGS _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), evmlog_module_name, __FUNCTION__
#define EVMLOG_WITH_HEADER_NORMAL_FORMAT "[%05ld.%06ld|%d|%s] "
#define EVMLOG_WITH_HEADER_NORMAL_ARGS _5DIGIT_SECS(ts), _6DIGIT_USECS(ts), (int)syscall(SYS_gettid), evmlog_module_name

#define EVMLOG_NO_HEADER_DEBUG_FORMAT "%s:%d %s(): "
#define EVMLOG_NO_HEADER_DEBUG_ARGS __FILE__, __LINE__, __FUNCTION__
#define EVMLOG_NO_HEADER_TRACE_FORMAT "%s(): "
#define EVMLOG_NO_HEADER_TRACE_ARGS __FUNCTION__
#define EVMLOG_NO_HEADER_NORMAL_FORMAT "%s"
#define EVMLOG_NO_HEADER_NORMAL_ARGS ""

#define evm_log_system_error(format, args...) {\
	char buf[1024];\
	strerror_r(errno, buf, 1024);\
	if (evmlog_add_header) {\
		struct timespec ts;\
		clock_gettime(CLOCK_MONOTONIC, &ts);\
		if (evmlog_use_syslog) {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				syslog(LOG_ERR, EVMLOG_WITH_HEADER_DEBUG_FORMAT "%s >> " format, EVMLOG_WITH_HEADER_DEBUG_ARGS, buf, ##args);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				syslog(LOG_ERR, EVMLOG_WITH_HEADER_TRACE_FORMAT "%s >> " format, EVMLOG_WITH_HEADER_TRACE_ARGS, buf, ##args);\
			} else {\
				syslog(LOG_ERR, EVMLOG_WITH_HEADER_NORMAL_FORMAT "%s >> " format, EVMLOG_WITH_HEADER_NORMAL_ARGS, buf, ##args);\
			}\
		} else {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				fprintf(stderr, EVMLOG_WITH_HEADER_DEBUG_FORMAT "%s >> " format, EVMLOG_WITH_HEADER_DEBUG_ARGS, buf, ##args);\
				fflush(stderr);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				fprintf(stderr, EVMLOG_WITH_HEADER_TRACE_FORMAT "%s >> " format, EVMLOG_WITH_HEADER_TRACE_ARGS, buf, ##args);\
				fflush(stderr);\
			} else {\
				fprintf(stderr, EVMLOG_WITH_HEADER_NORMAL_FORMAT "%s >> " format, EVMLOG_WITH_HEADER_NORMAL_ARGS, buf, ##args);\
				fflush(stderr);\
			}\
		}\
	} else {\
		if (evmlog_use_syslog) {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				syslog(LOG_ERR, EVMLOG_NO_HEADER_DEBUG_FORMAT "%s >> " format, EVMLOG_NO_HEADER_DEBUG_ARGS, buf, ##args);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				syslog(LOG_ERR, EVMLOG_NO_HEADER_TRACE_FORMAT "%s >> " format, EVMLOG_NO_HEADER_TRACE_ARGS, buf, ##args);\
			} else {\
				syslog(LOG_ERR, EVMLOG_NO_HEADER_NORMAL_FORMAT "%s >> " format, EVMLOG_NO_HEADER_NORMAL_ARGS, buf, ##args);\
			}\
		} else {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				fprintf(stderr, EVMLOG_NO_HEADER_DEBUG_FORMAT "%s >> " format, EVMLOG_NO_HEADER_DEBUG_ARGS, buf, ##args);\
				fflush(stderr);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				fprintf(stderr, EVMLOG_NO_HEADER_TRACE_FORMAT "%s >> " format, EVMLOG_NO_HEADER_TRACE_ARGS, buf, ##args);\
				fflush(stderr);\
			} else {\
				fprintf(stderr, EVMLOG_NO_HEADER_NORMAL_FORMAT "%s >> " format, EVMLOG_NO_HEADER_NORMAL_ARGS, buf, ##args);\
				fflush(stderr);\
			}\
		}\
	}\
}

#define evm_log_error(format, args...) {\
	if (evmlog_add_header) {\
		struct timespec ts;\
		clock_gettime(CLOCK_MONOTONIC, &ts);\
		if (evmlog_use_syslog) {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				syslog(LOG_ERR, EVMLOG_WITH_HEADER_DEBUG_FORMAT format, EVMLOG_WITH_HEADER_DEBUG_ARGS, ##args);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				syslog(LOG_ERR, EVMLOG_WITH_HEADER_TRACE_FORMAT format, EVMLOG_WITH_HEADER_TRACE_ARGS, ##args);\
			} else {\
				syslog(LOG_ERR, EVMLOG_WITH_HEADER_NORMAL_FORMAT format, EVMLOG_WITH_HEADER_NORMAL_ARGS, ##args);\
			}\
		} else {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				fprintf(stderr, EVMLOG_WITH_HEADER_DEBUG_FORMAT format, EVMLOG_WITH_HEADER_DEBUG_ARGS, ##args);\
				fflush(stderr);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				fprintf(stderr, EVMLOG_WITH_HEADER_TRACE_FORMAT format, EVMLOG_WITH_HEADER_TRACE_ARGS, ##args);\
				fflush(stderr);\
			} else {\
				fprintf(stderr, EVMLOG_WITH_HEADER_NORMAL_FORMAT format, EVMLOG_WITH_HEADER_NORMAL_ARGS, ##args);\
				fflush(stderr);\
			}\
		}\
	} else {\
		if (evmlog_use_syslog) {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				syslog(LOG_ERR, EVMLOG_NO_HEADER_DEBUG_FORMAT format, EVMLOG_NO_HEADER_DEBUG_ARGS, ##args);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				syslog(LOG_ERR, EVMLOG_NO_HEADER_TRACE_FORMAT format, EVMLOG_NO_HEADER_TRACE_ARGS, ##args);\
			} else {\
				syslog(LOG_ERR, EVMLOG_NO_HEADER_NORMAL_FORMAT format, EVMLOG_NO_HEADER_NORMAL_ARGS, ##args);\
			}\
		} else {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				fprintf(stderr, EVMLOG_NO_HEADER_DEBUG_FORMAT format, EVMLOG_NO_HEADER_DEBUG_ARGS, ##args);\
				fflush(stderr);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				fprintf(stderr, EVMLOG_NO_HEADER_TRACE_FORMAT format, EVMLOG_NO_HEADER_TRACE_ARGS, ##args);\
				fflush(stderr);\
			} else {\
				fprintf(stderr, EVMLOG_NO_HEADER_NORMAL_FORMAT format, EVMLOG_NO_HEADER_NORMAL_ARGS, ##args);\
				fflush(stderr);\
			}\
		}\
	}\
}

#define evm_log_warning(format, args...) {\
	if (evmlog_add_header) {\
		struct timespec ts;\
		clock_gettime(CLOCK_MONOTONIC, &ts);\
		if (evmlog_use_syslog) {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				syslog(LOG_WARNING, EVMLOG_WITH_HEADER_DEBUG_FORMAT format, EVMLOG_WITH_HEADER_DEBUG_ARGS, ##args);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				syslog(LOG_WARNING, EVMLOG_WITH_HEADER_TRACE_FORMAT format, EVMLOG_WITH_HEADER_TRACE_ARGS, ##args);\
			} else if (evmlog_normal || evmlog_verbose) {\
				syslog(LOG_WARNING, EVMLOG_WITH_HEADER_NORMAL_FORMAT format, EVMLOG_WITH_HEADER_NORMAL_ARGS, ##args);\
			}\
		} else {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				fprintf(stdout, EVMLOG_WITH_HEADER_DEBUG_FORMAT format, EVMLOG_WITH_HEADER_DEBUG_ARGS, ##args);\
				fflush(stdout);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				fprintf(stdout, EVMLOG_WITH_HEADER_TRACE_FORMAT format, EVMLOG_WITH_HEADER_TRACE_ARGS, ##args);\
				fflush(stdout);\
			} else if (evmlog_normal || evmlog_verbose) {\
				fprintf(stdout, EVMLOG_WITH_HEADER_NORMAL_FORMAT format, EVMLOG_WITH_HEADER_NORMAL_ARGS, ##args);\
				fflush(stdout);\
			}\
		}\
	} else {\
		if (evmlog_use_syslog) {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				syslog(LOG_WARNING, EVMLOG_NO_HEADER_DEBUG_FORMAT format, EVMLOG_NO_HEADER_DEBUG_ARGS, ##args);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				syslog(LOG_WARNING, EVMLOG_NO_HEADER_TRACE_FORMAT format, EVMLOG_NO_HEADER_TRACE_ARGS, ##args);\
			} else if (evmlog_normal || evmlog_verbose) {\
				syslog(LOG_WARNING, EVMLOG_NO_HEADER_NORMAL_FORMAT format, EVMLOG_NO_HEADER_NORMAL_ARGS, ##args);\
			}\
		} else {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				fprintf(stdout, EVMLOG_NO_HEADER_DEBUG_FORMAT format, EVMLOG_NO_HEADER_DEBUG_ARGS, ##args);\
				fflush(stdout);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				fprintf(stdout, EVMLOG_NO_HEADER_TRACE_FORMAT format, EVMLOG_NO_HEADER_TRACE_ARGS, ##args);\
				fflush(stdout);\
			} else if (evmlog_normal || evmlog_verbose) {\
				fprintf(stdout, EVMLOG_NO_HEADER_NORMAL_FORMAT format, EVMLOG_NO_HEADER_NORMAL_ARGS, ##args);\
				fflush(stdout);\
			}\
		}\
	}\
}

#define evm_log_notice(format, args...) {\
	if (evmlog_add_header) {\
		struct timespec ts;\
		clock_gettime(CLOCK_MONOTONIC, &ts);\
		if (evmlog_use_syslog) {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				syslog(LOG_NOTICE, EVMLOG_WITH_HEADER_DEBUG_FORMAT format, EVMLOG_WITH_HEADER_DEBUG_ARGS, ##args);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				syslog(LOG_NOTICE, EVMLOG_WITH_HEADER_TRACE_FORMAT format, EVMLOG_WITH_HEADER_TRACE_ARGS, ##args);\
			} else if (evmlog_normal || evmlog_verbose) {\
				syslog(LOG_NOTICE, EVMLOG_WITH_HEADER_NORMAL_FORMAT format, EVMLOG_WITH_HEADER_NORMAL_ARGS, ##args);\
			}\
		} else {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				fprintf(stdout, EVMLOG_WITH_HEADER_DEBUG_FORMAT format, EVMLOG_WITH_HEADER_DEBUG_ARGS, ##args);\
				fflush(stdout);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				fprintf(stdout, EVMLOG_WITH_HEADER_TRACE_FORMAT format, EVMLOG_WITH_HEADER_TRACE_ARGS, ##args);\
				fflush(stdout);\
			} else if (evmlog_normal || evmlog_verbose) {\
				fprintf(stdout, EVMLOG_WITH_HEADER_NORMAL_FORMAT format, EVMLOG_WITH_HEADER_NORMAL_ARGS, ##args);\
				fflush(stdout);\
			}\
		}\
	} else {\
		if (evmlog_use_syslog) {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				syslog(LOG_NOTICE, EVMLOG_NO_HEADER_DEBUG_FORMAT format, EVMLOG_NO_HEADER_DEBUG_ARGS, ##args);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				syslog(LOG_NOTICE, EVMLOG_NO_HEADER_TRACE_FORMAT format, EVMLOG_NO_HEADER_TRACE_ARGS, ##args);\
			} else if (evmlog_normal || evmlog_verbose) {\
				syslog(LOG_NOTICE, EVMLOG_NO_HEADER_NORMAL_FORMAT format, EVMLOG_NO_HEADER_NORMAL_ARGS, ##args);\
			}\
		} else {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				fprintf(stdout, EVMLOG_NO_HEADER_DEBUG_FORMAT format, EVMLOG_NO_HEADER_DEBUG_ARGS, ##args);\
				fflush(stdout);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				fprintf(stdout, EVMLOG_NO_HEADER_TRACE_FORMAT format, EVMLOG_NO_HEADER_TRACE_ARGS, ##args);\
				fflush(stdout);\
			} else if (evmlog_normal || evmlog_verbose) {\
				fprintf(stdout, EVMLOG_NO_HEADER_NORMAL_FORMAT format, EVMLOG_NO_HEADER_NORMAL_ARGS, ##args);\
				fflush(stdout);\
			}\
		}\
	}\
}

#define evm_log_info(format, args...) {\
	if (evmlog_add_header) {\
		struct timespec ts;\
		clock_gettime(CLOCK_MONOTONIC, &ts);\
		if (evmlog_use_syslog) {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				syslog(LOG_INFO, EVMLOG_WITH_HEADER_DEBUG_FORMAT format, EVMLOG_WITH_HEADER_DEBUG_ARGS, ##args);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				syslog(LOG_INFO, EVMLOG_WITH_HEADER_TRACE_FORMAT format, EVMLOG_WITH_HEADER_TRACE_ARGS, ##args);\
			} else if (evmlog_verbose) {\
				syslog(LOG_INFO, EVMLOG_WITH_HEADER_NORMAL_FORMAT format, EVMLOG_WITH_HEADER_NORMAL_ARGS, ##args);\
			}\
		} else {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				fprintf(stdout, EVMLOG_WITH_HEADER_DEBUG_FORMAT format, EVMLOG_WITH_HEADER_DEBUG_ARGS, ##args);\
				fflush(stdout);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				fprintf(stdout, EVMLOG_WITH_HEADER_TRACE_FORMAT format, EVMLOG_WITH_HEADER_TRACE_ARGS, ##args);\
				fflush(stdout);\
			} else if (evmlog_verbose) {\
				fprintf(stdout, EVMLOG_WITH_HEADER_NORMAL_FORMAT format, EVMLOG_WITH_HEADER_NORMAL_ARGS, ##args);\
				fflush(stdout);\
			}\
		}\
	} else {\
		if (evmlog_use_syslog) {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				syslog(LOG_INFO, EVMLOG_NO_HEADER_DEBUG_FORMAT format, EVMLOG_NO_HEADER_DEBUG_ARGS, ##args);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				syslog(LOG_INFO, EVMLOG_NO_HEADER_TRACE_FORMAT format, EVMLOG_NO_HEADER_TRACE_ARGS, ##args);\
			} else if (evmlog_verbose) {\
				syslog(LOG_INFO, EVMLOG_NO_HEADER_NORMAL_FORMAT format, EVMLOG_NO_HEADER_NORMAL_ARGS, ##args);\
			}\
		} else {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				fprintf(stdout, EVMLOG_NO_HEADER_DEBUG_FORMAT format, EVMLOG_NO_HEADER_DEBUG_ARGS, ##args);\
				fflush(stdout);\
			} else if (EVMLOG_MODULE_TRACE && evmlog_trace) {\
				fprintf(stdout, EVMLOG_NO_HEADER_TRACE_FORMAT format, EVMLOG_NO_HEADER_TRACE_ARGS, ##args);\
				fflush(stdout);\
			} else if (evmlog_verbose) {\
				fprintf(stdout, EVMLOG_NO_HEADER_NORMAL_FORMAT format, EVMLOG_NO_HEADER_NORMAL_ARGS, ##args);\
				fflush(stdout);\
			}\
		}\
	}\
}

#define evm_log_debug(format, args...) {\
	if (evmlog_add_header) {\
		struct timespec ts;\
		clock_gettime(CLOCK_MONOTONIC, &ts);\
		if (evmlog_use_syslog) {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				syslog(LOG_DEBUG, EVMLOG_WITH_HEADER_DEBUG_FORMAT format, EVMLOG_WITH_HEADER_DEBUG_ARGS, ##args);\
			}\
		} else {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				fprintf(stdout, EVMLOG_WITH_HEADER_DEBUG_FORMAT format, EVMLOG_WITH_HEADER_DEBUG_ARGS, ##args);\
				fflush(stdout);\
			}\
		}\
	} else {\
		if (evmlog_use_syslog) {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				syslog(LOG_DEBUG, EVMLOG_NO_HEADER_DEBUG_FORMAT format, EVMLOG_NO_HEADER_DEBUG_ARGS, ##args);\
			}\
		} else {\
			if (EVMLOG_MODULE_DEBUG && evmlog_debug) {\
				fprintf(stdout, EVMLOG_NO_HEADER_DEBUG_FORMAT format, EVMLOG_NO_HEADER_DEBUG_ARGS, ##args);\
				fflush(stdout);\
			}\
		}\
	}\
}

#define evm_log_return_system_err(msg, args...) {\
	int errsv = errno;\
	evm_log_system_error(msg, ##args);\
	return -errsv;\
}

#define evm_log_return_err(msg, args...) {\
	int errsv = errno;\
	evm_log_error(msg, ##args);\
	return -errsv;\
}

#endif /*EVM_FILE_log_module_h*/
