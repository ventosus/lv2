/*
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
   @defgroup uuid UUID

   Feature for getting unique integer identifiers, see
   <http://lv2plug.in/ns/ext/uuid> for details.

   @{
*/

#ifndef LV2_UUID_H
#define LV2_UUID_H

#define LV2_UUID_URI     "http://lv2plug.in/ns/ext/uuid"
#define LV2_UUID_PREFIX  LV2_UUID_URI "#"

#define LV2_UUID__get    LV2_UUID_PREFIX "get"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Opaque pointer to host data for LV2_UUID_Get.
*/
typedef void* LV2_UUID_Get_Handle;

/**
   Unique integer identifier.
*/
typedef int64_t LV2_UUID;

/**
   UUID Get Feature (LV2_UUID__get)
*/
typedef struct _LV2_UUID_Get {
	/**
	   Opaque pointer to host data.

	   This MUST be passed to next() whenever it is called.
	   Otherwise, it must not be interpreted in any way.
	*/
	LV2_UUID_Get_Handle handle;

	/**
	   Get a unique integer identifier.

	   This function MUST be RT-safe, non-blocking, lock-free, wait-free, etc.

	   @param handle MUST be the callback_data member of this struct.
	*/
	LV2_UUID (*next)(LV2_UUID_Get_Handle handle);
} LV2_UUID_Get;

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* LV2_UUID_H */

/**
   @}
*/
