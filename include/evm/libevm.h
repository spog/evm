/*
 * The public API of the event machine (libevm) library
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

#ifndef libevm_h
#define libevm_h

#include <errno.h>
#if 0
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif
#include <sys/uio.h>
#include <sys/epoll.h>
#include <time.h>

/* Struct aliases */
typedef struct evm_init evm_init_struct;
typedef struct evm_sigpost evm_sigpost_struct;
typedef struct evm_msgs_link evm_msgs_link_struct;
typedef struct evm_tmrs_link evm_tmrs_link_struct;
typedef struct evm_tab evm_tab_struct;
typedef struct evm_fd evm_fd_struct;
typedef struct evm_ids evm_ids_struct;
typedef struct evm_message evm_message_struct;
typedef struct evm_timer evm_timer_struct;

#ifdef evm_c
/* PRIVATE usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN
#else
/* PUBLIC usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN extern
#endif

EXTERN unsigned int evm_version_major;
EXTERN unsigned int evm_version_minor;
EXTERN unsigned int evm_version_patch;

/*User provided initialization structure!*/
struct evm_init {
	struct evm_sigpost *evm_sigpost;
	int evm_relink;
	evm_msgs_link_struct *evm_msgs_link;
	evm_tmrs_link_struct *evm_tmrs_link;
	int evm_msgs_link_max;
	int evm_tmrs_link_max;
	evm_tab_struct *evm_msgs_tab;
	evm_tab_struct *evm_tmrs_tab;
	struct epoll_event *epoll_events;
	int epoll_max_events;
	int epoll_nfds;
	int epollfd;
	void *msg_queue; /*internal message queue*/
	void *tmr_queue; /*internal timer queue*/
	void *priv; /*private - application specific data related to this machine*/
};

/*User provided signal post-handling EVM callbacks!*/
struct evm_sigpost {
	int (*sigpost_handle)(int signum, void *ev_ptr);
};

/*Elements of the user provided event type information and callbacks!*/
struct evm_msgs_link {
	int first_ev_id;
	int last_ev_id;
	int linked_msgs;
	evm_message_struct *msgs_tab;
	int (*ev_type_parse)(void *ev_ptr);
};

struct evm_tmrs_link {
	int first_ev_id;
	int last_ev_id;
	int linked_tmrs;
	evm_timer_struct *tmrs_tab;
	int (*ev_type_parse)(void *ev_ptr);
};

/*Elements of the user provided event machine table!*/
struct evm_tab {
	unsigned int ev_type;
	int ev_id;
	int (*ev_prepare)(void *ev_ptr);
	int (*ev_handle)(void *ev_ptr);
	int (*ev_finalize)(void *ev_ptr);
};

struct evm_fd {
	int fd;
	unsigned int ev_type;
	struct epoll_event ev_epoll;
	evm_message_struct *msg_ptr;
	int (*msg_receive)(int fd, evm_message_struct *msg_ptr);
#if 0
	int (*msg_send)(int sock, struct sockaddr_in *sockAddr, const char *buffer);
#endif
};

struct evm_ids {
	unsigned int ev_id;
	unsigned int evm_idx;
};

EXTERN int evm_init(evm_init_struct *evm_init_ptr);
EXTERN int evm_run(evm_init_struct *evm_init_ptr);

#ifdef evm_messages_c
/* PRIVATE usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN
#else
/* PUBLIC usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN extern
#endif

struct evm_message {
	evm_init_struct *evm_ptr;
	int saved;
	void *ctx_ptr;
	evm_ids_struct msg_ids;
	int rval_decode;
	void *msg_decode;
#if 0
	struct sockaddr_in msg_addr;
#endif
	struct iovec iov_buff;
};

EXTERN int evm_message_fd_add(evm_init_struct *evm_init_ptr, evm_fd_struct *evm_fd_ptr);
EXTERN int evm_message_call(evm_init_struct *evm_init_ptr, evm_message_struct *msg);
EXTERN int evm_message_pass(evm_init_struct *evm_init_ptr, evm_message_struct *msg);

EXTERN int evm_message_concatenate(const void *buffer, size_t size, void *msgBuf);

#ifdef evm_timers_c
/* PRIVATE usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN
#else
/* PUBLIC usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN extern
#endif
struct evm_timer {
	evm_init_struct *evm_ptr;
	int saved;
	int stopped;
	void *ctx_ptr;
	evm_ids_struct tmr_ids;
	struct timespec tm_stamp;
	evm_timer_struct *next;
};

EXTERN evm_timer_struct * evm_timer_start(evm_init_struct *evm_init_ptr, evm_ids_struct tmr_evm_ids, time_t tv_sec, long tv_nsec, void *ctx_ptr);
EXTERN int evm_timer_stop(evm_timer_struct *timer);
EXTERN int evm_timer_finalize(void *ptr);

#endif /*libevm_h*/
