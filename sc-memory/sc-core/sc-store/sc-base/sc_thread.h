/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef _sc_thread_h_
#define _sc_thread_h_

#include "threads.h"

//typedef GThread sc_thread;
typedef thrd_t sc_thread;

//#define sc_thread_self g_thread_self
#define sc_thread_self thrd_current

#endif
