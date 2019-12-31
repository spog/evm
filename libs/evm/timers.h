/*
 * The EVM timers module
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

#ifndef EVM_FILE_timers_h
#define EVM_FILE_timers_h

#ifdef EVM_FILE_timers_c
/* PRIVATE usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN
#else
/* PUBLIC usage of the PUBLIC part. */
#	undef EXTERN
#	define EXTERN extern
#endif

struct tmrs_queue {
	evm_timer_struct *first_tmr;
	evm_timer_struct *last_tmr;
	pthread_mutex_t access_mutex;
}; /*tmrs_queue_struct*/

/*
 * Per consumer timers initialization.
 * Return:
 * - Pointer to initialized tmrs_queue
 * - NULL on failure
 */
EXTERN tmrs_queue_struct * timers_queue_init(evm_consumer_struct *consumer_ptr);
EXTERN evm_timer_struct * timers_check(evm_consumer_struct *consumer_ptr);

#endif /*EVM_FILE_timers_h*/
