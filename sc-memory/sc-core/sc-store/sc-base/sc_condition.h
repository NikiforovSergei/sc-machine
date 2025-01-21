/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef _sc_condition_h_
#define _sc_condition_h_

#include "threads.h"

//typedef GCond sc_condition;
typedef pthread_cond_t sc_condition;

//#define sc_cond_init(condition) g_cond_init(condition)
#define sc_cond_init(condition) cnd_init(condition)

//#define sc_cond_wait(condition, mutex) g_cond_wait(condition, mutex)
#define sc_cond_wait(condition, mutex) cnd_wait(condition, mutex)

//#define sc_cond_signal(condition) g_cond_signal(condition)
#define sc_cond_signal(condition) cnd_signal(condition)

//#define sc_cond_broadcast(condition) g_cond_broadcast(condition)
#define sc_cond_broadcast(condition) cnd_broadcast(condition)

//#define sc_cond_destroy(condition) g_cond_clear(condition)
#define sc_cond_destroy(condition) cnd_destroy(condition)

#endif
