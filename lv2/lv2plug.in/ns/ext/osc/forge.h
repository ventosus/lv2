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
   @file forge.h An API for constructing LV2 OSC atoms.

   This file provides an API for constructing Atoms which makes it relatively
   simple to build nested atoms of arbitrary complexity without requiring
   dynamic memory allocation.

   The API is based on successively appending the appropriate pieces to build a
   complete Atom.  The size of containers is automatically updated.  Functions
   that begin a container return (via their frame argument) a stack frame which
   must be popped when the container is finished.

   All output is written to a user-provided buffer or sink function.  This
   makes it popssible to create create atoms on the stack, on the heap, in LV2
   port buffers, in a ringbuffer, or elsewhere, all using the same API.

   This entire API is realtime safe if used with a buffer or a realtime safe
   sink, except lv2_atom_forge_init() which is only realtime safe if the URI
   map function is.

   Note these functions are all static inline, do not take their address.

   This header is non-normative, it is provided for convenience.
*/

/**
   @defgroup forge Forge
   @ingroup atom
   @{
*/

#ifndef LV2_OSC_FORGE_H
#define LV2_OSC_FORGE_H

#define __USE_MISC // bloody hack for C89
#include <endian.h>
#include <ctype.h>

#include "lv2/lv2plug.in/ns/ext/osc/osc.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A "forge" for creating atoms by appending to a buffer. */
typedef struct {
	LV2_Atom_Forge *forge;

	LV2_URID Packet;
	LV2_URID Bundle;
	LV2_URID Message;
	LV2_URID Timestamp;

	LV2_URID bundleTimestamp;
	LV2_URID bundleItems;

	LV2_URID messagePath;
	LV2_URID messageArguments;

	LV2_URID MidiEvent;
} LV2_OSC_Forge;

typedef enum {
	LV2_OSC_INT				= 'i', /**< 32-bit Integer */
	LV2_OSC_FLOAT			= 'f', /**< 32-bit Integer */
	LV2_OSC_STRING		= 's', /**< 32-bit Integer */
	LV2_OSC_BLOB			= 'b', /**< 32-bit Integer */

	LV2_OSC_LONG			= 'h', /**< 32-bit Integer */
	LV2_OSC_DOUBLE		= 'd', /**< 32-bit Integer */
	LV2_OSC_TIMESTAMP	= 't', /**< 32-bit Integer */

	LV2_OSC_TRUE			= 'T', /**< 32-bit Integer */
	LV2_OSC_FALSE			= 'F', /**< 32-bit Integer */
	LV2_OSC_NIL				= 'N', /**< 32-bit Integer */
	LV2_OSC_IMPULSE		= 'I', /**< 32-bit Integer */

	LV2_OSC_SYMBOL		= 'S', /**< 32-bit Integer */
	LV2_OSC_MIDI			= 'm' /**< 32-bit Integer */
} LV2_OSC_Argument_Type;

/**
   Initialise `forge`.

   URIs will be mapped using `map` and stored, a reference to `map` itself is
   not held.
*/
static inline void
lv2_osc_forge_init(LV2_OSC_Forge *oforge, LV2_URID_Map* map)
{
	oforge->Packet = map->map(map->handle, LV2_OSC__Packet);
	oforge->Bundle = map->map(map->handle, LV2_OSC__Bundle);
	oforge->Message = map->map(map->handle, LV2_OSC__Message);
	oforge->Timestamp = map->map(map->handle, LV2_OSC__Timestamp);

	oforge->bundleTimestamp = map->map(map->handle, LV2_OSC__bundleTimestamp);
	oforge->bundleItems = map->map(map->handle, LV2_OSC__bundleItems);

	oforge->messagePath = map->map(map->handle, LV2_OSC__messagePath);
	oforge->messageArguments = map->map(map->handle, LV2_OSC__messageArguments);

	oforge->MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
}

static inline bool
lv2_osc_forge_is_packet_type(LV2_OSC_Forge *oforge, LV2_URID type)
{
	return type == oforge->Packet;
}

static inline bool
lv2_osc_forge_is_bundle_type(LV2_OSC_Forge *oforge, LV2_URID type)
{
	return type == oforge->Bundle;
}

static inline bool
lv2_osc_forge_is_message_type(LV2_OSC_Forge *oforge, LV2_URID type)
{
	return type == oforge->Message;
}

static inline bool
lv2_osc_forge_bundle_unpack(LV2_OSC_Forge *oforge, const LV2_Atom_Object *obj,
	const LV2_OSC_Timestamp **timestamp, const LV2_Atom_Tuple **items)
{
	*timestamp = NULL;
	*items = NULL;

	LV2_Atom_Object_Query q [] = {
		{ oforge->bundleTimestamp, (const LV2_Atom **)timestamp },
		{ oforge->bundleItems, (const LV2_Atom **)items },
		{ 0, NULL }
	};
	lv2_atom_object_query(obj, q);

	if(!*timestamp || ((*timestamp)->atom.type != oforge->Timestamp))
		return false;
	if(!*items || ((*items)->atom.type != oforge->forge->Tuple))
		return false;

	return true;
}

static inline bool
lv2_osc_forge_message_unpack(LV2_OSC_Forge *oforge, const LV2_Atom_Object *obj,
	const LV2_Atom_String **path, const LV2_Atom_Tuple **arguments)
{
	*path = NULL;
	*arguments = NULL;

	LV2_Atom_Object_Query q [] = {
		{ oforge->messagePath, (const LV2_Atom **)path },
		{ oforge->messageArguments, (const LV2_Atom **)arguments },
		{ 0, NULL }
	};
	lv2_atom_object_query(obj, q);

	if(!*path || ((*path)->atom.type != oforge->forge->String))
		return false;
	if(arguments && ((*arguments)->atom.type != oforge->forge->Tuple))
		return false;

	return true;
}

/**
   TODO
*/
static inline void
lv2_osc_forge_set_forge(LV2_OSC_Forge *oforge, LV2_Atom_Forge *forge)
{
	oforge->forge = forge;
}

#define lv2_osc_forge_int(oforge, value) \
	lv2_atom_forge_int((oforge)->forge, (value))

#define lv2_osc_forge_float(oforge, value) \
	lv2_atom_forge_float((oforge)->forge, (value))

#define lv2_osc_forge_string(oforge, value, size) \
	lv2_atom_forge_string((oforge)->forge, (value), (size))

#define lv2_osc_forge_long(oforge, value) \
	lv2_atom_forge_long((oforge)->forge, (value))

#define lv2_osc_forge_double(oforge, value) \
	lv2_atom_forge_double((oforge)->forge, (value))

#define lv2_osc_forge_bool(oforge, value) \
	lv2_atom_forge_bool((oforge)->forge, (value))

#define lv2_osc_forge_bool(oforge, value) \
	lv2_atom_forge_bool((oforge)->forge, (value))

#define lv2_osc_forge_true(oforge) \
	lv2_osc_forge_bool((oforge), 1)

#define lv2_osc_forge_false(oforge) \
	lv2_osc_forge_bool((oforge), 1)

#define lv2_osc_forge_nil(oforge) \
	lv2_atom_forge_atom((oforge)->forge, 0, 0)

#define lv2_osc_forge_impulse(oforge) \
	lv2_atom_forge_impulse((oforge)->forge)

#define lv2_osc_forge_symbol(oforge, value) \
	lv2_atom_forge_urid((oforge)->forge, (value))

static inline LV2_Atom_Forge_Ref
lv2_osc_forge_chunk(LV2_OSC_Forge* oforge, LV2_URID type, const uint8_t *buf, uint32_t size)
{
	LV2_Atom_Forge_Ref ref;
	if(  (ref = lv2_atom_forge_atom(oforge->forge, size, type))
		&& (ref = lv2_atom_forge_raw(oforge->forge, buf, size)) ) {
		lv2_atom_forge_pad(oforge->forge, size);
	}
	return ref;
}

static inline LV2_Atom_Forge_Ref
lv2_osc_forge_midi(LV2_OSC_Forge* oforge, const uint8_t *buf, uint32_t size)
{
	return lv2_osc_forge_chunk(oforge, oforge->MidiEvent, buf, size);
}

static inline LV2_Atom_Forge_Ref
lv2_osc_forge_blob(LV2_OSC_Forge* oforge, const uint8_t *buf, uint32_t size)
{
	return lv2_osc_forge_chunk(oforge, oforge->forge->Chunk, buf, size);
}

/**
   TODO
*/
static inline LV2_Atom_Forge_Ref
lv2_osc_forge_timestamp(LV2_OSC_Forge* oforge, uint32_t integral, uint32_t fraction)
{
	const LV2_OSC_Timestamp a = {
		{ sizeof(LV2_OSC_Timestamp), oforge->Timestamp },
		{ integral, fraction } };
	return lv2_atom_forge_primitive(oforge->forge, &a.atom);
}

/**
   TODO
*/
static inline LV2_Atom_Forge_Ref
lv2_osc_forge_bundle(LV2_OSC_Forge* oforge,
	LV2_Atom_Forge_Frame frame [2],
	LV2_URID id,
	uint32_t integral,
	uint32_t fraction)
{
	LV2_Atom_Forge_Ref ref;
	if(  (ref = lv2_atom_forge_object(oforge->forge, &frame[0], id, oforge->Bundle))
		&& (ref = lv2_atom_forge_key(oforge->forge, oforge->bundleTimestamp))
		&& (ref = lv2_osc_forge_timestamp(oforge, integral, fraction))
		&& (ref = lv2_atom_forge_key(oforge->forge, oforge->bundleItems))
		&& (ref = lv2_atom_forge_tuple(oforge->forge, &frame[1])) ) {
		return ref;
	}
	return 0;
}

// characters not allowed in OSC path string
static const char invalid_path_chars [] = {
	' ', '#',
	'\0'
};

// allowed characters in OSC format string
static const char valid_format_chars [] = {
	LV2_OSC_INT, LV2_OSC_FLOAT, LV2_OSC_STRING, LV2_OSC_BLOB,
	LV2_OSC_TRUE, LV2_OSC_FALSE, LV2_OSC_NIL, LV2_OSC_IMPULSE,
	LV2_OSC_LONG, LV2_OSC_DOUBLE, LV2_OSC_TIMESTAMP,
	LV2_OSC_SYMBOL, LV2_OSC_MIDI,
	'\0'
};

// check for valid path string
static inline int
lv2_osc_check_path(const char *path)
{
	if(path[0] != '/')
		return 0;

	for(const char *ptr=path+1; *ptr!='\0'; ptr++)
		if( (isprint(*ptr) == 0) || (strchr(invalid_path_chars, *ptr) != NULL) )
			return 0;

	return 1;
}

// check for valid format string 
static inline int
lv2_osc_check_fmt(const char *format, int offset)
{
	if(offset)
	{
		if(format[0] != ',')
			return 0;
	}

	for(const char *ptr=format+offset; *ptr!='\0'; ptr++)
	{
		if(strchr(valid_format_chars, *ptr) == NULL)
			return 0;
	}

	return 1;
}

/**
   TODO
*/
static inline LV2_Atom_Forge_Ref
lv2_osc_forge_message(LV2_OSC_Forge* oforge,
	LV2_Atom_Forge_Frame frame [2],
	LV2_URID id,
	const char *path)
{
	LV2_Atom_Forge_Ref ref;
	if(!lv2_osc_check_path(path))
		return 0;
	if(  (ref = lv2_atom_forge_object(oforge->forge, &frame[0], id, oforge->Message))
		&& (ref = lv2_atom_forge_key(oforge->forge, oforge->messagePath))
		&& (ref = lv2_atom_forge_string(oforge->forge, path, strlen(path)))
		&& (ref = lv2_atom_forge_key(oforge->forge, oforge->messageArguments))
		&& (ref = lv2_atom_forge_tuple(oforge->forge, &frame[1])) ) {
		return ref;
	}
	return 0;
}

/**
   TODO
*/
static inline void
lv2_osc_forge_pop(LV2_OSC_Forge* oforge,
	LV2_Atom_Forge_Frame frame [2])
{
	lv2_atom_forge_pop(oforge->forge, &frame[1]); // a LV2_Atom_Tuple
	lv2_atom_forge_pop(oforge->forge, &frame[0]); // a LV2_Atom_Object
}

static inline LV2_Atom_Forge_Ref
lv2_osc_forge_message_varlist(LV2_OSC_Forge *oforge, uint32_t id,
	const char *path, const char *fmt, va_list args)
{
	LV2_Atom_Forge_Frame frame [2];
	LV2_Atom_Forge_Ref ref;

	if(!lv2_osc_check_fmt(fmt, 0))
		return 0;
	if(!(ref = lv2_osc_forge_message(oforge, frame, id, path)))
		return 0;

	for(const char *type = fmt; *type; type++)
	{
		switch(*type)
		{
			case LV2_OSC_INT:
			{
				if(!(ref = lv2_osc_forge_int(oforge, va_arg(args, int32_t))))
					return 0;
				break;
			}
			case LV2_OSC_FLOAT:
			{
				if(!(ref = lv2_osc_forge_float(oforge, (float)va_arg(args, double))))
					return 0;
				break;
			}
			case LV2_OSC_STRING:
			{
				const char *s = va_arg(args, const char *);
				if(!s || !(ref = lv2_osc_forge_string(oforge, s, strlen(s))))
					return 0;
				break;
			}
			case LV2_OSC_SYMBOL:
			{
				if(!(ref = lv2_osc_forge_symbol(oforge, va_arg(args, uint32_t))))
					return 0;
				break;
			}
			case LV2_OSC_BLOB:
			{
				const int32_t size = va_arg(args, int32_t);
				const uint8_t *b = va_arg(args, const uint8_t *);
				if(!b || !(ref = lv2_osc_forge_blob(oforge, b, size)))
					return 0;
				break;
			}
			
			case LV2_OSC_LONG:
			{
				if(!(ref = lv2_osc_forge_long(oforge, va_arg(args, int64_t))))
					return 0;
				break;
			}
			case LV2_OSC_DOUBLE:
			{
				if(!(ref = lv2_osc_forge_double(oforge, va_arg(args, double))))
					return 0;
				break;
			}
			case LV2_OSC_TIMESTAMP:
			{
				const uint32_t integral = va_arg(args, uint32_t);
				const uint32_t fraction = va_arg(args, uint32_t);
				if(!(ref = lv2_osc_forge_timestamp(oforge, integral, fraction)))
					return 0;
				break;
			}
			
			case LV2_OSC_MIDI:
			{
				const int32_t size = va_arg(args, int32_t);
				const uint8_t *m = va_arg(args, const uint8_t *);
				if(!m || !(ref = lv2_osc_forge_midi(oforge, m, size)))
					return 0;
				break;
			}
			
			case LV2_OSC_TRUE:
			{
				if(!(ref = lv2_osc_forge_true(oforge)))
					return 0;
				break;
			}
			case LV2_OSC_FALSE:
			{
				if(!(ref = lv2_osc_forge_false(oforge)))
					return 0;
				break;
			}
			case LV2_OSC_NIL:
			{
				if(!(ref = lv2_osc_forge_nil(oforge)))
					return 0;
				break;
			}
			case LV2_OSC_IMPULSE:
			{
				if(!(ref = lv2_osc_forge_impulse(oforge)))
					return 0;
				break;
			}

			default: // unknown argument type
			{
				return 0;
			}
		}
	}

	lv2_osc_forge_pop(oforge, frame);

	return ref;
}

static inline LV2_Atom_Forge_Ref
lv2_osc_forge_message_vararg(LV2_OSC_Forge *oforge, uint32_t id,
	const char *path, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	LV2_Atom_Forge_Ref ref;
	ref = lv2_osc_forge_message_varlist(oforge, id, path, fmt, args);

	va_end(args);

	return ref;
}

/**
   @}
   @}
*/

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* LV2_OSC_FORGE_H */
