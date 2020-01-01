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

#ifndef EVM_FILE_libevm_h
#define EVM_FILE_libevm_h

#include <errno.h>
#include <sys/uio.h>
#include <sys/epoll.h>
#include <time.h>
#include <semaphore.h>

/*
 * Common
 */
/* Struct aliases */
typedef struct evm_sigpost evm_sigpost_struct;
typedef struct evm_fd evm_fd_struct;

extern unsigned int evm_version_major;
extern unsigned int evm_version_minor;
extern unsigned int evm_version_patch;

typedef void evmStruct;
typedef void evmMsgtypeStruct;
typedef void evmMsgidStruct;
typedef void evmTmridStruct;
typedef void evmConsumerStruct;
typedef void evmTopicStruct;
typedef void evmTimerStruct;
typedef void evmMessageStruct;

/*User provided signal post-handling EVM callbacks!*/
struct evm_sigpost {
	int (*sigpost_handle)(int signum, void *ev_ptr);
}; /*evm_sigpost_struct*/

struct evm_fd {
	int fd;
	evmMsgtypeStruct *msgtype_ptr;
	struct epoll_event ev_epoll;
	evmMessageStruct *msg_ptr;
	int (*msg_receive)(int fd, evmMessageStruct *msg_ptr);
#if 0
	int (*msg_send)(int sock, struct sockaddr_in *sockAddr, const char *buffer);
#endif
}; /*evm_fd_struct*/

/*
 * Public API functions:
 */

/*
 * Public API function:
 * - evm_init()
 *
 * Main event machine initialization
 */
extern evmStruct * evm_init(void);

/*
 * Functions: evm_objectX_add()
 * Returns:
 * - NULL, if adding object fails or (evm == NULL)
 * - pointer to the existing object with required id
 * - pointer to the new object with required id
 */
extern evmMsgtypeStruct * evm_msgtype_add(evmStruct *evm, int id);
extern evmMsgidStruct * evm_msgid_add(evmMsgtypeStruct *msgtype, int id);
extern evmTmridStruct * evm_tmrid_add(evmStruct *evm, int id);
extern evmConsumerStruct * evm_consumer_add(evmStruct *evm, int id);
extern evmTopicStruct * evm_topic_add(evmStruct *evm, int id);

/*
 * Functions: evm_objectX_get()
 * Returns:
 * - NULL, if objectX with required id wasn't found or (evm == NULL)
 * - pointer to the existing objectX with required id
 */
extern evmMsgtypeStruct * evm_msgtype_get(evmStruct *evm, int id);
extern evmMsgidStruct * evm_msgid_get(evmMsgtypeStruct *msgtype, int id);
extern evmTmridStruct * evm_tmrid_get(evmStruct *evm, int id);
extern evmConsumerStruct * evm_consumer_get(evmStruct *evm, int id);
extern evmTopicStruct * evm_topic_get(evmStruct *evm, int id);

/*
 * Function: evm_objectX_del()
 * Returns:
 * - NULL, if objectX with required id wasn't found or (evm == NULL)
 * - pointer to the deleted (freed) objectX with required id
 */
extern evmMsgtypeStruct * evm_msgtype_del(evmStruct *evm, int id);
extern evmMsgidStruct * evm_msgid_del(evmMsgtypeStruct *msgtype, int id);
extern evmTmridStruct * evm_tmrid_del(evmStruct *evm, int id);
extern evmConsumerStruct * evm_consumer_del(evmStruct *evm, int id);
extern evmTopicStruct * evm_topic_del(evmStruct *evm, int id);

extern int evm_priv_set(evmStruct *evm, void *priv);
extern void * evm_priv_get(evmStruct *evm);
extern int evm_sigpost_set(evmStruct *evm, evm_sigpost_struct *sigpost);

/*
 * Public API functions:
 * - evm_run_once()
 * - evm_run_async()
 * - evm_run()
 */
extern int evm_run_once(evmConsumerStruct *consumer);
extern int evm_run_async(evmConsumerStruct *consumer);
extern int evm_run(evmConsumerStruct *consumer);

/*
 * Public API functions:
 * - evm_consumer_priv_set()
 * - evm_consumer_priv_get()
 */
extern int evm_consumer_priv_set(evmConsumerStruct *consumer, void *priv);
extern void * evm_consumer_priv_get(evmConsumerStruct *consumer);

/*
 * Messages
 */

/*
 * Public API functions:
 * - evm_msgtype_add()
 * - evm_msgtype_get()
 * - evm_msgtype_del()
 */
extern evmMsgtypeStruct * evm_msgtype_add(evmStruct *evm, int type);
extern evmMsgtypeStruct * evm_msgtype_get(evmStruct *evm, int type);
extern evmMsgtypeStruct * evm_msgtype_del(evmStruct *evm, int type);

/*
 * Public API function:
 * - evm_msgtype_cb_parse_set()
 */
extern int evm_msgtype_cb_parse_set(evmMsgtypeStruct *msgtype, int (*msgtype_parse)(void *ptr));

/*
 * Public API functions:
 * - evm_msgid_cb_prepare_set()
 * - evm_msgid_cb_handle_set()
 * - evm_msgid_cb_finalize_set()
 */
extern int evm_msgid_cb_prepare_set(evmMsgidStruct *msgid_ptr, int (*msg_prepare)(void *ptr));
extern int evm_msgid_cb_handle_set(evmMsgidStruct *msgid_ptr, int (*msg_handle)(void *ptr));
extern int evm_msgid_cb_finalize_set(evmMsgidStruct *msgid_ptr, int (*msg_finalize)(void *ptr));

extern int evm_message_fd_add(evmStruct *evm_ptr, evm_fd_struct *evm_fd_ptr);

/*
 * Public API functions:
 * - evm_message_new()
 * - evm_message_delete()
 * - evm_message_consumer_get()
 * - evm_message_ctx_set()
 * - evm_message_ctx_get()
 * - evm_message_iovec_set()
 * - evm_message_iovec_get()
 */
extern evmMessageStruct * evm_message_new(evmMsgtypeStruct *msgtype, evmMsgidStruct *msgid);
extern void evm_message_delete(evmMessageStruct *msg);
extern evmConsumerStruct * evm_message_consumer_get(evmMessageStruct *msg);
extern int evm_message_ctx_set(evmMessageStruct *msg, void *ctx);
extern void * evm_message_ctx_get(evmMessageStruct *msg);
extern int evm_message_iovec_set(evmMessageStruct *msg, struct iovec *iov_buff);
extern struct iovec * evm_message_iovec_get(evmMessageStruct *msg);

/*
 * Public API functions:
 * - evm_message_pass()
 * - evm_message_concatenate()
 */
extern int evm_message_pass(evmConsumerStruct *consumer, evmMessageStruct *msg);
extern int evm_message_concatenate(const void *buffer, size_t size, void *msgBuf);

/*
 * Timers
 */

/*
 * Public API functions:
 * - evm_tmrid_add()
 * - evm_tmrid_get()
 * - evm_tmrid_del()
 */
extern evmTmridStruct * evm_tmrid_add(evmStruct *evm, int id);
extern evmTmridStruct * evm_tmrid_get(evmStruct *evm, int id);
extern evmTmridStruct * evm_tmrid_del(evmStruct *evm, int id);

/*
 * Public API functions:
 * - evm_tmrid_cb_handle_set()
 * - evm_tmrid_cb_finalize_set()
 */
extern int evm_tmrid_cb_handle_set(evmTmridStruct *tmrid, int (*tmr_handle)(void *ptr));
extern int evm_tmrid_cb_finalize_set(evmTmridStruct *tmrid, int (*tmr_finalize)(void *ptr));

/*
 * Public API functions:
 * - evm_timer_start()
 * - evm_timer_stop()
 * - evm_timer_consumer_get()
 * - evm_timer_ctx_get()
 */
extern evmTimerStruct * evm_timer_start(evmConsumerStruct *consumer, evmTmridStruct *tmrid, time_t tv_sec, long tv_nsec, void *ctx);
extern int evm_timer_stop(evmTimerStruct *timer);
extern evmConsumerStruct * evm_timer_consumer_get(evmTimerStruct *timer);
extern void * evm_timer_ctx_get(evmTimerStruct *timer);

/*
 * Public API function:
 * - evm_timer_finalize()
 *
 * Automatically free expired timer, if specific "timerid_finalize_cb" not set!
 * Requires the "ptr" argument to be the timer_ptr itself!
 */
extern int evm_timer_finalize(void *ptr);

#endif /*EVM_FILE_libevm_h*/
