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
#include <sys/poll.h>
#include <time.h>

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
struct evm_init_struct {
	struct evm_sigpost_struct *evm_sigpost;
	struct evm_link_struct *evm_link;
	int evm_link_max;
	struct evm_tab_struct *evm_tab;
	struct evm_fds_struct *evm_fds;
};

/*User provided signal post-handling EVM callbacks!*/
struct evm_sigpost_struct {
	int (*sigpost_handle)(int signum, void *ev_ptr);
};

/*Elements of the user provided event type linkage callbacks!*/
struct evm_link_struct {
	int (*ev_type_parse)(void *ev_ptr);
	int (*ev_type_link)(int ev_id, int ev_idx);
};

/*Elements of the user provided event machine table!*/
struct evm_tab_struct {
	unsigned int ev_type;
	int ev_id;
	int (*ev_prepare)(void *ev_ptr);
	int (*ev_handle)(void *ev_ptr);
	int (*ev_finalize)(void *ev_ptr);
};

#define FDS_TAB_SIZE 10
struct evm_fds_struct {
	nfds_t nfds;
	unsigned int ev_type_fds[FDS_TAB_SIZE];
	struct pollfd ev_poll_fds[FDS_TAB_SIZE];
	struct message_struct *msg_ptrs[FDS_TAB_SIZE];
	int (*msg_receive[FDS_TAB_SIZE])(int fd, struct message_struct *msg_ptr);
#if 0
	int (*msg_receive[FDS_TAB_SIZE])(int socket, struct message_struct *msg_ptr);
	int (*msg_send[FDS_TAB_SIZE])(int sock, struct sockaddr_in *sockAddr, const char *buffer);
#endif
};

struct evm_ids {
	unsigned int ev_id;
	unsigned int evm_idx;
};

EXTERN int evm_link_init(struct evm_init_struct *evm_init_ptr);
EXTERN int evm_init(struct evm_init_struct *evm_init_ptr);
EXTERN int evm_run(struct evm_init_struct *evm_init_ptr);

EXTERN void evm_message_pass(struct message_struct *msg);

#ifdef messages_c
/* PRIVATE usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN
#else
/* PUBLIC usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN extern
#endif

#define MAX_RECV_SIZE 1024
#define MAX_SEND_SIZE MAX_RECV_SIZE

struct message_struct {
	struct evm_tab_struct *evm_tab;
	int saved;
	int fds_index;
	void *ctx_ptr;
	struct evm_ids msg_ids;
	struct message_struct *next_msg;
	int rval_decode;
	void *msg_decode;
#if 0
	struct sockaddr_in msg_addr;
#endif
	int recv_size;
	char recv_buff[MAX_RECV_SIZE];
};

EXTERN int message_concatenate(const void *buffer, size_t size, void *msgBuf);

#ifdef timers_c
/* PRIVATE usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN
#else
/* PUBLIC usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN extern
#endif
struct timer_struct {
	struct evm_tab_struct *evm_tab;
	int saved;
	int stopped;
	void *ctx_ptr;
	struct evm_ids tmr_ids;
	struct timespec tm_stamp;
	struct timer_struct *next_tmr;
};

struct timers_struct {
	struct timer_struct *first_tmr;
};

EXTERN struct timer_struct * start_timer(struct evm_tab_struct *evm_tab, struct evm_ids tmr_evm_ids, time_t tv_sec, long tv_nsec, void *ctx_ptr);
EXTERN int stop_timer(struct timer_struct *timer);
EXTERN int evm_finalize_timer(void *ptr);


#endif /*libevm_h*/
