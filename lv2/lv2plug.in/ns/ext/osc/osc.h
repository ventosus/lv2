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

#ifndef LV2_OSC_H
#define LV2_OSC_H

#include <stdint.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"

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

/**
   TODO
*/
typedef enum {
	LV2_OSC_INT				= 'i', /**< 32-bit Integer */
	LV2_OSC_FLOAT			= 'f', /**< 32-bit Float */
	LV2_OSC_STRING		= 's', /**< String */
	LV2_OSC_BLOB			= 'b', /**< Blob */

	LV2_OSC_LONG			= 'h', /**< 64-bit Integer */
	LV2_OSC_DOUBLE		= 'd', /**< 64-bit Double precision float */
	LV2_OSC_TIMESTAMP	= 't', /**< 32.32 Fixedpoint timestamp */

	LV2_OSC_TRUE			= 'T', /**< True */
	LV2_OSC_FALSE			= 'F', /**< False */
	LV2_OSC_NIL				= 'N', /**< Nil */
	LV2_OSC_IMPULSE		= 'I', /**< Impulse */

	LV2_OSC_SYMBOL		= 'S', /**< Symbol */
	LV2_OSC_MIDI			= 'm'  /**< 3-byte MIDI message */
} LV2_OSC_Argument_Type;

/**
   TODO
*/
union _swap32 {
	uint32_t u;
	int32_t i;
	float f;
};

/**
   TODO
*/
union _swap64 {
	uint64_t u;
	int64_t h;
	double d;
};

/**
   TODO
*/
typedef struct {
	LV2_URID_Map *map;

	LV2_URID OSC_Packet;
	LV2_URID OSC_Bundle;
	LV2_URID OSC_Message;
	LV2_URID OSC_Timestamp;

	LV2_URID OSC_bundleTimestamp;
	LV2_URID OSC_bundleItems;

	LV2_URID OSC_messagePath;
	LV2_URID OSC_messageArguments;

	LV2_URID MIDI_MidiEvent;

	LV2_URID ATOM_Int;
	LV2_URID ATOM_Long;
	LV2_URID ATOM_String;
	LV2_URID ATOM_Float;
	LV2_URID ATOM_Double;
	LV2_URID ATOM_URID;
	LV2_URID ATOM_Bool;
	LV2_URID ATOM_Tuple;
	LV2_URID ATOM_Object;
	LV2_URID ATOM_Chunk;
	LV2_URID ATOM_Impulse;
} LV2_OSC;

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

/** Pad a size to 32 bits. */
static inline uint32_t
lv2_osc_pad_size(uint32_t size)
{
	return (size + 3U) & (~3U);
}

/**
   TODO
*/
static inline void
lv2_osc_init(LV2_OSC *osc, LV2_URID_Map *map)
{
	osc->map = map;

	osc->OSC_Packet = map->map(map->handle, LV2_OSC__Packet);
	osc->OSC_Bundle = map->map(map->handle, LV2_OSC__Bundle);
	osc->OSC_Message = map->map(map->handle, LV2_OSC__Message);
	osc->OSC_Timestamp = map->map(map->handle, LV2_OSC__Timestamp);

	osc->OSC_bundleTimestamp = map->map(map->handle, LV2_OSC__bundleTimestamp);
	osc->OSC_bundleItems = map->map(map->handle, LV2_OSC__bundleItems);

	osc->OSC_messagePath = map->map(map->handle, LV2_OSC__messagePath);
	osc->OSC_messageArguments = map->map(map->handle, LV2_OSC__messageArguments);

	osc->MIDI_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);

	osc->ATOM_Int = map->map(map->handle, LV2_ATOM__Int);
	osc->ATOM_Long = map->map(map->handle, LV2_ATOM__Long);
	osc->ATOM_String = map->map(map->handle, LV2_ATOM__String);
	osc->ATOM_Float = map->map(map->handle, LV2_ATOM__Float);
	osc->ATOM_Double = map->map(map->handle, LV2_ATOM__Double);
	osc->ATOM_URID = map->map(map->handle, LV2_ATOM__URID);
	osc->ATOM_Bool = map->map(map->handle, LV2_ATOM__Bool);
	osc->ATOM_Tuple = map->map(map->handle, LV2_ATOM__Tuple);
	osc->ATOM_Object = map->map(map->handle, LV2_ATOM__Object);
	osc->ATOM_Chunk= map->map(map->handle, LV2_ATOM__Chunk);
	osc->ATOM_Impulse = map->map(map->handle, LV2_ATOM__Impulse);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV2_OSC_H */

/**
   @}
*/
