/*
 * The EVM messages module
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

#ifndef EVM_FILE_messages_h
#define EVM_FILE_messages_h

#ifdef EVM_FILE_messages_c
/* PRIVATE usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN
#else
/* PUBLIC usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN extern
#endif

typedef struct msg_hanger msg_hanger_struct;

struct msgs_queue {
	msg_hanger_struct *first_hanger;
	msg_hanger_struct *last_hanger;
//samo - orig:	evm_fd_struct *evmfd; /*internal message queue FD binding - eventfd()*/
	pthread_mutex_t access_mutex;
}; /*msgs_queue_struct*/

struct msg_hanger {
	msg_hanger_struct *next;
	msg_hanger_struct *prev;
	evm_message_struct *msg; /*hangs of a hanger when linked in a chain - i.e.: in a message queue*/
}; /*msg_hanger_struct*/

EXTERN msgs_queue_struct * messages_consumer_queue_init(evm_consumer_struct *consumer_ptr);
EXTERN msgs_queue_struct * messages_topic_queue_init(evm_topic_struct *topic_ptr);
EXTERN evm_message_struct * messages_check(evm_consumer_struct *consumer_ptr);

#endif /*EVM_FILE_messages_h*/
