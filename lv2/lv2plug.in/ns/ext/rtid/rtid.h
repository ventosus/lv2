/*
  Copyright 2008-2016 David Robillard <http://drobilla.net>
  Copyright 2016 Hanspeter Portner <dev@open-music-kontrollers.ch>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
   @defgroup rtid RTID

   Feature for getting unique integer identifiers, see
   <http://lv2plug.in/ns/ext/rtid> for details.

   @{
*/

#ifndef LV2_RTID_H
#define LV2_RTID_H

#define LV2_RTID_URI     "http://lv2plug.in/ns/ext/rtid"
#define LV2_RTID_PREFIX  LV2_RTID_URI "#"

#define LV2_RTID__get    LV2_RTID_PREFIX "get"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Opaque pointer to host data for LV2_RTID_Get.
*/
typedef void* LV2_RTID_Get_Handle;

/**
   Unique integer identifier.
*/
typedef int64_t LV2_RTID;

/**
   RTID Get Feature (LV2_RTID__get)
*/
typedef struct _LV2_RTID_Get {
	/**
	   Opaque pointer to host data.

	   This MUST be passed to next() whenever it is called.
	   Otherwise, it must not be interpreted in any way.
	*/
	LV2_RTID_Get_Handle handle;

	/**
	   Get a unique integer identifier.

	   This function MUST be RT-safe, non-blocking, lock-free and wait-free.
		 Hosts that cannot support lock-free 64-bit integers MAY fall back to
		 unsigned 32-bit integers.

	   @param handle MUST be the callback_data member of this struct.

	   @code
// Example host implementation in C11 with atomics

#include <stdatomic.h>

#if(ATOMIC_LONG_LOCK_FREE == 2)
#	pragma message("[RTID] atomic LONG is lock-free")
static atomic_long lv2_rtid_counter = ATOMIC_VAR_INIT(0);
#elif(ATOMIC_INT_LOCK_FREE == 2)
#	pragma message("[RTID] atomic INT is lock-free") 
static atomic_uint lv2_rtid_counter = ATOMIC_VAR_INIT(0);
#else
#	error "[RTID] neither atomic LONG nor INT is lock-free"
#endif

static LV2_RTID
lv2_rtid_get_next(LV2_RTID_Get_Handle handle)
{
#if(ATOMIC_LONG_LOCK_FREE == 2)
	atomic_long* counter = handle;
#elif(ATOMIC_INT_LOCK_FREE == 2)
	atomic_uint* counter = handle;
#endif
	return atomic_fetch_add_explicit(counter, 1, memory_order_relaxed);
}

static LV2_RTID_Get lv2_rtid_get = {
	.handle = &lv2_rtid_counter,
	.next = lv2_rtid_get_next
};

static const LV2_Feature lv2_rtid_get_feature = {
	.URI = LV2_RTID__get,
	.data = &lv2_rtid_get	
};
	   @endcode
	*/
	LV2_RTID (*next)(LV2_RTID_Get_Handle handle);
} LV2_RTID_Get;

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* LV2_RTID_H */

/**
   @}
*/
