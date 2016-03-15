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
   @defgroup osc OSC

   @{
*/

#ifndef LV2_OSC_RAZE_H
#define LV2_OSC_RAZE_H

#include "lv2/lv2plug.in/ns/ext/osc/mold.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int32_t size;
	uint8_t body [0];
} LV2_OSC_Bundle_Item;

typedef struct {
	char header [8];
	uint64_t timestamp;
	LV2_OSC_Bundle_Item item;
} LV2_OSC_Bundle;

static bool
lv2_osc_raze_is_bundle(const uint8_t *buf, size_t len)
{
	const LV2_OSC_Bundle *bundle = (const LV2_OSC_Bundle *)buf;

	if( (len < 20) || strncmp(bundle->header, "#bundle", 8) )
		return false;
	return true;
}

static bool
lv2_osc_raze_is_message(const uint8_t *buf, size_t len)
{
	if( (len < 8) || (buf[0] != '/') ) //TODO check path + fmt string
		return false;
	return true;
}

static void
lv2_osc_raze_bundle_timestamp_get(const LV2_OSC_Bundle *bundle, uint32_t *integral, uint32_t *fraction)
{
	union _swap64 s64 = { .u = bundle->timestamp };
	s64.u = be64toh(s64.u);
	*integral = s64.u >> 32;
	*fraction = s64.u & 0xffffffff;
}

static const LV2_OSC_Bundle_Item *
lv2_osc_raze_bundle_begin(const LV2_OSC_Bundle *bundle)
{
	return &bundle->item;
}

static const bool
lv2_osc_raze_bundle_is_end(const LV2_OSC_Bundle *bundle, size_t len, const LV2_OSC_Bundle_Item *iter)
{
	return (const uint8_t *)iter >= ((const uint8_t *)bundle + len);
}

static const LV2_OSC_Bundle_Item *
lv2_osc_raze_bundle_next(const LV2_OSC_Bundle_Item *iter)
{
	union _swap32 s32 = { .i = iter->size };
	s32.u = be32toh(s32.u);
	return (const LV2_OSC_Bundle_Item *)((const uint8_t *)iter + sizeof(int32_t) + s32.i);
}

#define LV2_OSC_RAZE_BUNDLE_FOREACH(bundle, size, iter) \
	for(const LV2_OSC_Bundle_Item *(iter) = lv2_osc_raze_bundle_begin((bundle)); \
			!lv2_osc_raze_bundle_is_end((bundle), (size), (iter)); \
			(iter) = lv2_osc_raze_bundle_next((iter)))

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV2_OSC_RAZE_H */

/**
   @}
*/
