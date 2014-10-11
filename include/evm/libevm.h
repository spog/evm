/*
 *  Copyright (C) 2012  Samo Pogacnik
 */

/*
 * The public API of the event machine (libevm) library.
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
typedef struct evm_link evm_link_struct;
typedef struct evm_tab evm_tab_struct;
typedef struct evm_fd evm_fd_struct;
typedef struct evm_ids evm_ids_struct;
typedef struct evm_message evm_message_struct;
typedef struct evm_timer evm_timer_struct;
typedef struct evm_timers evm_timers_struct;

#ifdef evm_c
/* PRIVATE usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN
#else
/* PUBLIC usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN extern
#endif
/*User provided initialization structure!*/
struct evm_init {
	struct evm_sigpost *evm_sigpost;
	evm_link_struct *evm_link;
	int evm_link_max;
	evm_tab_struct *evm_tab;
	struct epoll_event *epoll_events;
	int epoll_max_events;
	int epoll_nfds;
	int epollfd;
	void *msg_queue; /*hidden-internal structure*/
};

/*User provided signal post-handling EVM callbacks!*/
struct evm_sigpost {
	int (*sigpost_handle)(int signum, void *ev_ptr);
};

/*Elements of the user provided event type linkage callbacks!*/
struct evm_link {
	int (*ev_type_parse)(void *ev_ptr);
	int (*ev_type_link)(int ev_id, int ev_idx);
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

EXTERN int evm_link_init(evm_init_struct *evm_init_ptr);
EXTERN int evm_init(evm_init_struct *evm_init_ptr);
EXTERN int evm_run(evm_init_struct *evm_init_ptr);

EXTERN int evm_message_fd_add(evm_init_struct *evm_init_ptr, evm_fd_struct *evm_fd_ptr);
EXTERN void evm_message_pass(evm_init_struct *evm_init_ptr, evm_message_struct *msg);

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
	evm_tab_struct *evm_tab;
	int saved;
	void *ctx_ptr;
	evm_ids_struct msg_ids;
	evm_message_struct *next_msg; /*when linked in a chain - i.e.: in a message queue*/
	int rval_decode;
	void *msg_decode;
#if 0
	struct sockaddr_in msg_addr;
#endif
	struct iovec iov_buff;
};

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
	evm_tab_struct *evm_tab;
	int saved;
	int stopped;
	void *ctx_ptr;
	evm_ids_struct tmr_ids;
	struct timespec tm_stamp;
	evm_timer_struct *next_tmr;
};

struct evm_timers {
	evm_timer_struct *first_tmr;
};

EXTERN evm_timer_struct * evm_timer_start(evm_tab_struct *evm_tab, evm_ids_struct tmr_evm_ids, time_t tv_sec, long tv_nsec, void *ctx_ptr);
EXTERN int evm_timer_stop(evm_timer_struct *timer);
EXTERN int evm_timer_finalize(void *ptr);

#endif /*libevm_h*/
