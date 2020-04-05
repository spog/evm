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
extern unsigned int evmVerMajor;
extern unsigned int evmVerMinor;
extern unsigned int evmVerPatch;

/* Struct aliases */
struct evm;
struct evm_msgtype;
struct evm_msgid;
struct evm_tmrid;
struct evm_consumer;
struct evm_topic;
struct evm_message;
struct evm_timer;
typedef struct evm evmStruct;
typedef struct evm_msgtype evmMsgtypeStruct;
typedef struct evm_msgid evmMsgidStruct;
typedef struct evm_tmrid evmTmridStruct;
typedef struct evm_consumer evmConsumerStruct;
typedef struct evm_topic evmTopicStruct;
typedef struct evm_message evmMessageStruct;
typedef struct evm_timer evmTimerStruct;

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
extern evmMsgtypeStruct * evm_msgtype_add(evmStruct *evm, int message_type_id);
extern evmMsgidStruct * evm_msgid_add(evmMsgtypeStruct *msgtype, int message_id);
extern evmTmridStruct * evm_tmrid_add(evmStruct *evm, int timer_id);
extern evmConsumerStruct * evm_consumer_add(evmStruct *evm, int consumer_id);
extern evmTopicStruct * evm_topic_add(evmStruct *evm, int topic_id);

/*
 * Functions: evm_objectX_get()
 * Returns:
 * - NULL, if objectX with required id wasn't found or (evm == NULL)
 * - pointer to the existing objectX with required id
 */
extern evmMsgtypeStruct * evm_msgtype_get(evmStruct *evm, int message_type_id);
extern evmMsgidStruct * evm_msgid_get(evmMsgtypeStruct *msgtype, int messsage_id);
extern evmTmridStruct * evm_tmrid_get(evmStruct *evm, int timer_id);
extern evmConsumerStruct * evm_consumer_get(evmStruct *evm, int consumer_id);
extern evmTopicStruct * evm_topic_get(evmStruct *evm, int topic_id);

/*
 * Function: evm_objectX_del()
 * Returns:
 * - NULL, if objectX with required id wasn't found or (evm == NULL)
 * - pointer to the deleted (freed) objectX with required id
 */
extern evmMsgtypeStruct * evm_msgtype_del(evmStruct *evm, int message_type_id);
extern evmMsgidStruct * evm_msgid_del(evmMsgtypeStruct *msgtype, int message_id);
extern evmTmridStruct * evm_tmrid_del(evmStruct *evm, int timer_id);
extern evmConsumerStruct * evm_consumer_del(evmStruct *evm, int consumer_id);
extern evmTopicStruct * evm_topic_del(evmStruct *evm, int topic_id);

/*
 * Public API functions:
 * - evm_topic_subscribe()
 * - evm_topic_unsubscribe()
 */
/*
 * Function: evm_topic_subscribe()
 * Returns:
 * - NULL, if;
 *   - consumer is NULL
 *   - "evm" is not correctly initialized
 *   - topic with topic_id is not found
 * - Topic pointer, if:
 *   - topic is successfully subscribed by consumer
 *   - topic already subscribed by consumer
 */
evmTopicStruct * evm_topic_subscribe(evmConsumerStruct *consumer, int topic_id);
/*
 * Function: evm_topic_unsubscribe()
 * Returns:
 * - NULL, if;
 *   - consumer is NULL
 *   - "evm" is not correctly initialized
 *   - topic with topic_id is not found
 * - Topic pointer, if:
 *   - topic is successfully unsubscribed by consumer
 *   - topic already unsubscribed by consumer
 */
evmTopicStruct * evm_topic_unsubscribe(evmConsumerStruct *consumer, int topic_id);

extern int evm_priv_set(evmStruct *evm, void *priv);
extern void * evm_priv_get(evmStruct *evm);

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
 * - evm_msgid_cb_handle_set()
 */
extern int evm_msgid_cb_handle_set(evmMsgidStruct *msgid_ptr, int (*msg_handle)(evmConsumerStruct *consumer, evmMessageStruct *msg));

/*
 * Public API functions:
 * - evm_message_new()
 * - evm_message_delete()
 * - evm_message_alloc_add()
 * - evm_message_persistent_set()
 * - evm_message_ctx_set()
 * - evm_message_ctx_get()
 * - evm_message_data_get()
 * - evm_message_data_takeover()
 * - evm_message_lock()
 * - evm_message_unlock()
 */
extern evmMessageStruct * evm_message_new(evmMsgtypeStruct *msgtype, evmMsgidStruct *msgid, size_t size);
extern void evm_message_delete(evmMessageStruct *msg);
extern int evm_message_alloc_add(evmMessageStruct *msg, void *alloc);
extern int evm_message_persistent_set(evmMessageStruct *msg);
extern int evm_message_ctx_set(evmMessageStruct *msg, void *ctx);
extern void * evm_message_ctx_get(evmMessageStruct *msg);
extern void * evm_message_data_get(evmMessageStruct *msg);
extern void * evm_message_data_takeover(evmMessageStruct *msg);

/*
 * Public API functions:
 * - evm_message_pass()
 * - evm_message_post()
 */
extern int evm_message_pass(evmConsumerStruct *consumer, evmMessageStruct *msg);
extern int evm_message_post(evmTopicStruct *topic, evmMessageStruct *msg);

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
 */
extern int evm_tmrid_cb_handle_set(evmTmridStruct *tmrid, int (*tmr_handle)(evmConsumerStruct *consumer, evmTimerStruct *tmr));

/*
 * Public API functions:
 * - evm_timer_start()
 * - evm_timer_stop()
 * - evm_timer_ctx_set()
 * - evm_timer_ctx_get()
 */
extern evmTimerStruct * evm_timer_start(evmConsumerStruct *consumer, evmTmridStruct *tmrid, time_t tv_sec, long tv_nsec, void *ctx);
extern int evm_timer_stop(evmTimerStruct *timer);
extern int evm_timer_ctx_set(evmTimerStruct *timer, void *ctx);
extern void * evm_timer_ctx_get(evmTimerStruct *timer);

/*
 * Public API function:
 * - evm_timer_delete()
 *
 * Automatically frees expired timer after it has been handled!
 *  (OR)
 * Manually free previously stopped timer!
 * Requires the "timer_ptr" argument returned from "evm_timer_start()"!
 */
extern void evm_timer_delete(evmTimerStruct *tmr);

#endif /*EVM_FILE_libevm_h*/
