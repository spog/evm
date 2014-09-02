/*
 *  Copyright (C) 2012  Samo Pogacnik
 */

/*
 * The messages module
 */

#ifndef messages_h
#define messages_h

#ifdef messages_c
/* PRIVATE usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN
#else
/* PUBLIC usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN extern
#endif
/*
 * Here starts the PUBLIC stuff:
 *	Enter PUBLIC declarations using EXTERN!
 */
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>

#define COMP_MODULE libevm_module
EXTERN unsigned int COMP_MODULE;
#include "evm/log_conf.h"

#include "evm/libevm.h"

EXTERN int messages_init(struct evm_init_struct *evm_init_ptr);
EXTERN struct message_struct * messages_check(struct evm_init_struct *evm_init_ptr);

EXTERN void message_enqueue(struct message_struct *msg);
EXTERN struct message_struct * message_dequeue(void);

#ifdef messages_c
/*
 * Here is the PRIVATE stuff (within above ifdef).
 * It is here so we make sure, that the following PRIVATE stuff get included into own source ONLY!
 */
struct message_queue_struct {
	struct message_struct *first_msg;
	struct message_struct *last_msg;
};

static int messages_poll(struct evm_fds_struct *evm_fds, struct evm_sigpost_struct *evm_sigpost);
static int messages_receive(int fds_idx, struct evm_fds_struct *evm_fds, struct evm_link_struct *evm_linkage);
static int messages_parse(int fds_idx, struct evm_fds_struct *evm_fds, struct evm_link_struct *evm_linkage);

#endif /*messages_c*/
/*
 * Here continues the PUBLIC stuff, if necessary.
 */

#endif /*messages_h*/
