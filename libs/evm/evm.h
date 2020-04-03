/*
 * The event machine (evm) module
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

#ifndef EVM_FILE_evm_h
#define EVM_FILE_evm_h

#ifdef EVM_FILE_evm_c
/* PRIVATE usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN
#else
/* PUBLIC usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN extern
#endif

#define EVM_TRUE (0 == 0)
#define EVM_FALSE (0 != 0)

/*Generic EVM list head and element structures*/
typedef struct evmlist_head evmlist_head_struct;
typedef struct evmlist_el evmlist_el_struct;

/*Generic EVM list head structure*/
struct evmlist_head {
	pthread_mutex_t access_mutex;
	evmlist_el_struct *first;
}; /*evmlist_head_struct*/

/*Generic EVM list element structure*/
struct evmlist_el {
	evmlist_el_struct *prev;
	evmlist_el_struct *next;
	int id;
	void *el;
}; /*evmlist_el_struct*/

/*Specific structures*/
typedef struct evm evm_struct;
typedef struct evm_msgtype evm_msgtype_struct;
typedef struct evm_msgid evm_msgid_struct;
typedef struct evm_tmrid evm_tmrid_struct;
typedef struct evm_consumer evm_consumer_struct;
typedef struct evm_topic evm_topic_struct;
typedef struct evm_message evm_message_struct;
typedef struct evm_timer evm_timer_struct;

/*Structure returned by evm_init()!*/
struct evm {
	evmlist_head_struct *msgtypes_list;
	evmlist_head_struct *tmrids_list;
	evmlist_head_struct *consumers_list;
	evmlist_head_struct *topics_list;
#if 0 /*samo - orig*/
	evm_sigpost_struct *evm_sigpost;
#endif
	void *priv; /*private - application specific data*/
}; /*evm_struct*/

struct msgs_queue;
typedef struct msgs_queue msgs_queue_struct;

struct tmrs_queue;
typedef struct tmrs_queue tmrs_queue_struct;

struct evm_consumer {
	evm_struct *evm;
	int id;
	sem_t blocking_sem;
	msgs_queue_struct *msgs_queue; /*internal messages queue*/
	tmrs_queue_struct *tmrs_queue; /*internal timers queue*/
	void *priv; /*private - consumer specific data*/
}; /*evm_consumer_struct*/

struct evm_topic {
	evm_struct *evm;
	int id;
	evmlist_head_struct *consumers_list;
}; /*evm_topic_struct*/

/*
 * Messages
 */
struct evm_msgtype {
	evm_struct *evm;
	int id; /* 0, 1, 2, 3,... */
	int (*msgtype_parse)(void *ptr);
	evmlist_head_struct *msgids_list;
}; /*evm_msgtype_struct*/

struct evm_msgid {
	evm_struct *evm;
	evm_msgtype_struct *msgtype;
	int id;
	int (*msg_handle)(evm_consumer_struct *consumer, evm_message_struct *ptr);
}; /*evm_msgid_struct*/

struct evm_message {
	evmlist_head_struct *allocs_list;
	evm_msgtype_struct *msgtype;
	evm_msgid_struct *msgid;
	pthread_mutex_t amtx;
	int consumers;
	int saved;
	void *ctx;
	void *data;
}; /*evm_message_struct*/

/*
 * Timers
 */
struct evm_tmrid {
	evm_struct *evm;
	int id;
	int (*tmr_handle)(evm_consumer_struct *consumer, evm_timer_struct *ptr);
}; /*evm_tmrid_struct*/

struct evm_timer {
	evm_tmrid_struct *tmrid;
	pthread_mutex_t amtx;
	evm_consumer_struct *consumer;
	int saved;
	int stopped;
	void *ctx;
	struct timespec tm_stamp;
	evm_timer_struct *next;
}; /*evm_timer_struct*/

/*
 * Internally global "evmlist" helper functions:
 */
/*
 * evm_search_evmlist()
 * Returns:
 * - NULL, if list is empty (head->first == NULL)
 * - element with required id, if already existing
 * - last element, if required id not existing
 */
EXTERN evmlist_el_struct * evm_search_evmlist(evmlist_head_struct *head, int id);
/*
 * evm_check_evmlist()
 * Returns:
 * - NULL, if list is empty (head->first == NULL)
 * - element with required id, if already existing
 * - last element, if required id not existing
 */
EXTERN evmlist_el_struct * evm_check_evmlist(evmlist_head_struct *head, void *el);
/*
 * evm_new_evmlist_el()
 * Returns:
 * - NULL, if calloc() fails
 * - new element with required id set
 */
EXTERN evmlist_el_struct * evm_new_evmlist_el(int id);

#endif /*EVM_FILE_evm_h*/
