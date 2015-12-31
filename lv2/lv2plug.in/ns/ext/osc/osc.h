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

   Definitions of atomized OSC messages, see <http://lv2plug.in/ns/ext/osc>
   for details.
*/

#ifndef LV2_OSC_H
#define LV2_OSC_H

#include <stdint.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LV2_OSC_URI									"http://lv2plug.in/ns/ext/osc"
#define LV2_OSC_PREFIX							LV2_OSC_URI "#"

#define LV2_OSC__Event							LV2_OSC_PREFIX "Event"
#define LV2_OSC__Packet							LV2_OSC_PREFIX "Packet"
#define LV2_OSC__Bundle							LV2_OSC_PREFIX "Bundle"
#define LV2_OSC__Message						LV2_OSC_PREFIX "Message"
#define LV2_OSC__Timestamp					LV2_OSC_PREFIX "Timestamp"
#define LV2_OSC__bundleTimestamp		LV2_OSC_PREFIX "bundleTimestamp"
#define LV2_OSC__bundleItems				LV2_OSC_PREFIX "bundleItems"
#define LV2_OSC__messagePath				LV2_OSC_PREFIX "messagePath"
#define LV2_OSC__messageArguments		LV2_OSC_PREFIX "messageArguments"

/** The body of an osc:Timestamp. */
typedef struct {
	uint32_t integral; /**< Integral seconds since 1901-01-01. */
	uint32_t fraction; /**< Fraction of a second as multiple of 2e-32. */
} LV2_OSC_Timestamp_Body;

/** An osc:Timestamp. May be cast to LV2_Atom. */
typedef struct {
	LV2_Atom atom; /**< Atom header. */
	LV2_OSC_Timestamp_Body body; /**< Body. */
} LV2_OSC_Timestamp;

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* LV2_OSC_H */

/**
   @}
*/
