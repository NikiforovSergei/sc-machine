/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef _sc_mutex_h_
#define _sc_mutex_h_

//#include <glib.h>
#include "nuttx/mutex.h"
//#include "nuttx/threads.h"

//typedef GMutex sc_mutex;
typedef mutex_t sc_mutex;

//#define sc_mutex_init(mutex) g_mutex_init(mutex)
#define sc_mutex_init(mutex) nxmutex_init(mutex)

//#define sc_mutex_lock(mutex) g_mutex_lock(mutex)
#define sc_mutex_lock(mutex) nxmutex_lock(mutex)

//#define sc_mutex_unlock(mutex) g_mutex_unlock(mutex)
#define sc_mutex_unlock(mutex) nxmutex_unlock(mutex)

//#define sc_mutex_destroy(mutex) g_mutex_clear(mutex)
#define sc_mutex_destroy(mutex) nxmutex_destroy(mutex)

#endif
