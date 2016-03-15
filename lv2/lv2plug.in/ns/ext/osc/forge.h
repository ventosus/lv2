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
   @defgroup forge Forge
   @ingroup osc
   @{
*/

#ifndef LV2_OSC_FORGE_H
#define LV2_OSC_FORGE_H

#include <endian.h>
#include <ctype.h>

#include "lv2/lv2plug.in/ns/ext/osc/osc.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
   TODO
*/
#define lv2_osc_forge_int lv2_atom_forge_int
#define lv2_osc_forge_float lv2_atom_forge_float
#define lv2_osc_forge_string lv2_atom_forge_string
#define lv2_osc_forge_long lv2_atom_forge_long
#define lv2_osc_forge_double lv2_atom_forge_double
#define lv2_osc_forge_true(forge) lv2_atom_forge_bool((forge), 1)
#define lv2_osc_forge_false(forge) lv2_atom_forge_bool((forge), 0)
#define lv2_osc_forge_nil(forge) lv2_atom_forge_atom((forge), 0, 0)
#define lv2_osc_forge_impulse lv2_atom_forge_impulse

/**
   TODO
*/
static inline LV2_Atom_Forge_Ref
lv2_osc_forge_symbol(LV2_Atom_Forge *forge, LV2_OSC *osc, const char *symbol)
{
	const LV2_URID urid = osc->map->map(osc->map->handle, symbol);
	return lv2_atom_forge_urid(forge, urid);
}

/**
   TODO
*/
static inline LV2_Atom_Forge_Ref
lv2_osc_forge_chunk(LV2_Atom_Forge *forge, LV2_URID type,
	const uint8_t *buf, uint32_t size)
{
	LV2_Atom_Forge_Ref ref;
	if(  (ref = lv2_atom_forge_atom(forge, size, type))
		&& (ref = lv2_atom_forge_raw(forge, buf, size)) )
	{
		lv2_atom_forge_pad(forge, size);
		return ref;
	}
	return 0;
}

/**
   TODO
*/
static inline LV2_Atom_Forge_Ref
lv2_osc_forge_midi(LV2_Atom_Forge *forge, LV2_OSC *osc, const uint8_t *buf, uint32_t size)
{
	assert(size <= 3);
	return lv2_osc_forge_chunk(forge, osc->MIDI_MidiEvent, buf, size);
}

/**
   TODO
*/
static inline LV2_Atom_Forge_Ref
lv2_osc_forge_blob(LV2_Atom_Forge* forge, const uint8_t *buf, uint32_t size)
{
	return lv2_osc_forge_chunk(forge, forge->Chunk, buf, size);
}

/**
   TODO
*/
static inline LV2_Atom_Forge_Ref
lv2_osc_forge_timestamp(LV2_Atom_Forge *forge, LV2_OSC *osc,
	uint32_t integral, uint32_t fraction)
{
	const LV2_OSC_Timestamp a = {
		.atom = {
			.size = sizeof(LV2_OSC_Timestamp),
			.type = osc->OSC_Timestamp
		},
		.body = {
			.integral = integral,
			.fraction = fraction
		}
	};
	return lv2_atom_forge_primitive(forge, &a.atom);
}

/**
   TODO
*/
static inline LV2_Atom_Forge_Ref
lv2_osc_forge_bundle_head(LV2_Atom_Forge* forge, LV2_OSC *osc,
	LV2_Atom_Forge_Frame frame [2], LV2_URID id,
	uint32_t integral, uint32_t fraction)
{
	LV2_Atom_Forge_Ref ref;
	if(  (ref = lv2_atom_forge_object(forge, &frame[0], id, osc->OSC_Bundle))
		&& (ref = lv2_atom_forge_key(forge, osc->OSC_bundleTimestamp))
		&& (ref = lv2_osc_forge_timestamp(forge, osc, integral, fraction))
		&& (ref = lv2_atom_forge_key(forge, osc->OSC_bundleItems))
		&& (ref = lv2_atom_forge_tuple(forge, &frame[1])) )
	{
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

/**
   TODO
*/
static inline bool
lv2_osc_check_path(const char *path)
{
	if(path[0] != '/')
		return false;

	for(const char *ptr=path+1; *ptr!='\0'; ptr++)
		if( (isprint(*ptr) == 0) || (strchr(invalid_path_chars, *ptr) != NULL) )
			return false;

	return true;
}

/**
   TODO
*/
static inline bool
lv2_osc_check_fmt(const char *format, int offset)
{
	if(offset && (format[0] != ',') )
		return false;

	for(const char *ptr=format+offset; *ptr!='\0'; ptr++)
		if(strchr(valid_format_chars, *ptr) == NULL)
			return false;

	return true;
}

/**
   TODO
*/
static inline LV2_Atom_Forge_Ref
lv2_osc_forge_message_head(LV2_Atom_Forge *forge, LV2_OSC* osc,
	LV2_Atom_Forge_Frame frame [2],
	LV2_URID id,
	const char *path)
{
	LV2_Atom_Forge_Ref ref;
	if(  (ref = lv2_atom_forge_object(forge, &frame[0], id, osc->OSC_Message))
		&& (ref = lv2_atom_forge_key(forge, osc->OSC_messagePath))
		&& (ref = lv2_atom_forge_string(forge, path, strlen(path)))
		&& (ref = lv2_atom_forge_key(forge, osc->OSC_messageArguments))
		&& (ref = lv2_atom_forge_tuple(forge, &frame[1])) )
	{
		return ref;
	}
	return 0;
}

/**
   TODO
*/
static inline void
lv2_osc_forge_pop(LV2_Atom_Forge *forge, LV2_Atom_Forge_Frame frame [2])
{
	lv2_atom_forge_pop(forge, &frame[1]); // a LV2_Atom_Tuple
	lv2_atom_forge_pop(forge, &frame[0]); // a LV2_Atom_Object
}

/**
   TODO
*/
static inline LV2_Atom_Forge_Ref
lv2_osc_forge_message_varlist(LV2_Atom_Forge *forge, LV2_OSC *osc, uint32_t id,
	const char *path, const char *fmt, va_list args)
{
	LV2_Atom_Forge_Frame frame [2];
	LV2_Atom_Forge_Ref ref;

	if(!lv2_osc_check_path(path) || !lv2_osc_check_fmt(fmt, 0))
		return 0;
	if(!(ref = lv2_osc_forge_message_head(forge, osc, frame, id, path)))
		return 0;

	for(const char *type = fmt; *type; type++)
	{
		switch(*type)
		{
			case LV2_OSC_INT:
			{
				if(!(ref = lv2_osc_forge_int(forge, va_arg(args, int32_t))))
					return 0;
				break;
			}
			case LV2_OSC_FLOAT:
			{
				if(!(ref = lv2_osc_forge_float(forge, (float)va_arg(args, double))))
					return 0;
				break;
			}
			case LV2_OSC_STRING:
			{
				const char *s = va_arg(args, const char *);
				if(!s || !(ref = lv2_osc_forge_string(forge, s, strlen(s))))
					return 0;
				break;
			}
			case LV2_OSC_SYMBOL:
			{
				const char *S = va_arg(args, const char *);
				if(!S || !(ref = lv2_osc_forge_symbol(forge, osc, S)))
					return 0;
				break;
			}
			case LV2_OSC_BLOB:
			{
				const int32_t size = va_arg(args, int32_t);
				const uint8_t *b = va_arg(args, const uint8_t *);
				if(!b || !(ref = lv2_osc_forge_blob(forge, b, size)))
					return 0;
				break;
			}
			
			case LV2_OSC_LONG:
			{
				if(!(ref = lv2_osc_forge_long(forge, va_arg(args, int64_t))))
					return 0;
				break;
			}
			case LV2_OSC_DOUBLE:
			{
				if(!(ref = lv2_osc_forge_double(forge, va_arg(args, double))))
					return 0;
				break;
			}
			case LV2_OSC_TIMESTAMP:
			{
				const uint32_t integral = va_arg(args, uint32_t);
				const uint32_t fraction = va_arg(args, uint32_t);
				if(!(ref = lv2_osc_forge_timestamp(forge, osc, integral, fraction)))
					return 0;
				break;
			}
			
			case LV2_OSC_MIDI:
			{
				const int32_t size = va_arg(args, int32_t);
				const uint8_t *m = va_arg(args, const uint8_t *);
				if(!m || !(ref = lv2_osc_forge_midi(forge, osc, m, size)))
					return 0;
				break;
			}
			
			case LV2_OSC_TRUE:
			{
				if(!(ref = lv2_osc_forge_true(forge)))
					return 0;
				break;
			}
			case LV2_OSC_FALSE:
			{
				if(!(ref = lv2_osc_forge_false(forge)))
					return 0;
				break;
			}
			case LV2_OSC_NIL:
			{
				if(!(ref = lv2_osc_forge_nil(forge)))
					return 0;
				break;
			}
			case LV2_OSC_IMPULSE:
			{
				if(!(ref = lv2_osc_forge_impulse(forge)))
					return 0;
				break;
			}

			default: // unknown argument type
			{
				return 0;
			}
		}
	}

	lv2_osc_forge_pop(forge, frame);

	return ref;
}

/**
   TODO
*/
static inline LV2_Atom_Forge_Ref
lv2_osc_forge_message_vararg(LV2_Atom_Forge *forge, LV2_OSC *osc, uint32_t id,
	const char *path, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	LV2_Atom_Forge_Ref ref;
	ref = lv2_osc_forge_message_varlist(forge, osc, id, path, fmt, args);

	va_end(args);

	return ref;
}

static inline LV2_Atom_Forge_Ref
lv2_osc_forge_message(LV2_Atom_Forge *forge, LV2_OSC *osc,
	uint32_t id, const uint8_t *buf, size_t size)
{
	LV2_Atom_Forge_Frame frame [2];
	LV2_Atom_Forge_Ref ref;

	const void *ptr = buf;
	const void *end = buf + size;

	const char *path = (const char *)ptr;
	ptr += lv2_osc_pad_size(strlen(path) + 1);
	if(ptr > end)
		return 0;

	const char *fmt = (const char *)ptr;
	ptr += lv2_osc_pad_size(strlen(fmt) + 1);
	if(ptr > end)
		return 0;

	if(!lv2_osc_check_path(path) || !lv2_osc_check_fmt(fmt, 1))
		return 0;

	if(!(ref = lv2_osc_forge_message_head(forge, osc, frame, id, path)))
		return 0;

	for(const char *type = fmt+1; *type && (ptr < end); type++)
	{
		switch(*type)
		{
			case LV2_OSC_INT:
			{
				union _swap32 s = { .u = *(const uint32_t *)ptr };
				s.u = be32toh(s.u);
				if(!(ref = lv2_osc_forge_int(forge, s.i)))
					return 0;
				ptr += 4;
				break;
			}
			case LV2_OSC_FLOAT:
			{
				union _swap32 s = { .u = *(const uint32_t *)ptr };
				s.u = be32toh(s.u);
				if(!(ref = lv2_osc_forge_float(forge, s.f)))
					return 0;
				ptr += 4;
				break;
			}
			case LV2_OSC_STRING:
			{
				const char *s = ptr;
				const uint32_t len = strlen(s);
				if(!(ref = lv2_osc_forge_string(forge, s, len)))
					return 0;
				ptr += lv2_osc_pad_size(len + 1);
				break;
			}
			case LV2_OSC_BLOB:
			{
				union _swap32 s = { .u = *(const uint32_t *)ptr };
				s.u = be32toh(s.u);
				const int32_t len = s.i;
				if(len < 0)
					return 0;
				const uint8_t *b = ptr + 4;
				if(!(ref = lv2_osc_forge_blob(forge, b, len)))
					return 0;
				ptr += 4 + lv2_osc_pad_size(len);
				break;
			}
			
			case LV2_OSC_LONG:
			{
				union _swap64 s = { .u = *(const uint64_t *)ptr };
				s.u = be64toh(s.u);
				if(!(ref = lv2_osc_forge_long(forge, s.h)))
					return 0;
				ptr += 8;
				break;
			}
			case LV2_OSC_DOUBLE:
			{
				union _swap64 s = { .u = *(const uint64_t *)ptr };
				s.u = be64toh(s.u);
				if(!(ref = lv2_osc_forge_double(forge, s.d)))
					return 0;
				ptr += 8;
				break;
			}
			case LV2_OSC_TIMESTAMP:
			{
				union _swap64 s = { .u = *(const uint64_t *)ptr };
				s.u = be64toh(s.u);
				const uint32_t integral = s.u >> 32;
				const uint32_t fraction = s.u & 0xffffffff;
				if(!(ref = lv2_osc_forge_timestamp(forge, osc, integral, fraction)))
					return 0;
				ptr += 8;
				break;
			}
			
			case LV2_OSC_TRUE:
			{
				if(!(ref = lv2_osc_forge_true(forge)))
					return 0;
				break;
			}
			case LV2_OSC_FALSE:
			{
				if(!(ref = lv2_osc_forge_false(forge)))
					return 0;
				break;
			}
			case LV2_OSC_NIL:
			{
				if(!(ref = lv2_osc_forge_nil(forge)))
					return 0;
				break;
			}
			case LV2_OSC_IMPULSE:
			{
				if(!(ref = lv2_osc_forge_impulse(forge)))
					return 0;
				break;
			}
			
			case LV2_OSC_SYMBOL:
			{
				const char *S = ptr;
				const uint32_t len = strlen(S);
				if(!(ref = lv2_osc_forge_symbol(forge, osc, S)))
					return 0;
				ptr += lv2_osc_pad_size(len + 1);
				break;
			}
			case LV2_OSC_MIDI:
			{
				const uint8_t *m = ptr + 1; // skip port
				if(!(ref = lv2_osc_forge_midi(forge, osc, m, 3)))
					return 0;
				ptr += 4;
				break;
			}

			default: // unknown argument type
			{
				return 0;
			}
		}
	}

	lv2_osc_forge_pop(forge, frame);

	if(ptr == end)
		return ref;

	return ref;
}

static inline LV2_Atom_Forge_Ref
lv2_osc_forge_packet(LV2_Atom_Forge *forge, LV2_OSC *osc,
	uint32_t id, const uint8_t *buf, size_t size);

static inline LV2_Atom_Forge_Ref
lv2_osc_forge_bundle(LV2_Atom_Forge *forge, LV2_OSC *osc,
	uint32_t id, const uint8_t *buf, size_t size)
{
	LV2_Atom_Forge_Frame frame [2];
	LV2_Atom_Forge_Ref ref;

	const void *ptr = buf;
	const void *end = buf + size;

	if(ptr + 16 > end)
		return 0;

	if(strncmp(ptr, "#bundle", 8))
		return 0;
	ptr += 8;

	union _swap64 s64 = { .u = *(const uint64_t *)ptr };
	s64.u = be64toh(s64.u);
	const uint32_t integral = s64.u >> 32;
	const uint32_t fraction = s64.u & 0xffffffff;
	ptr += 8;

	if(!(ref = lv2_osc_forge_bundle_head(forge, osc, frame, id, integral, fraction)))
		return 0;

	while(ptr + 4 <= end)
	{
		union _swap32 s32 = { .u = *(const uint32_t *)ptr };
		s32.u = be32toh(s32.u);
		ptr += 4;

		if( (s32.i < 0) || (s32.i & 0x3) || (ptr + s32.i > end) )
			return 0;

		if(!(ref = lv2_osc_forge_packet(forge, osc, id, ptr, s32.i)))
			return 0;
		ptr += s32.i;
	}

	lv2_osc_forge_pop(forge, frame);

	if(ptr == end)
		return ref;

	return 0;
}

static inline LV2_Atom_Forge_Ref
lv2_osc_forge_packet(LV2_Atom_Forge *forge, LV2_OSC *osc,
	uint32_t id, const uint8_t *buf, size_t size)
{
	LV2_Atom_Forge_Ref ref;
	if(  (ref = lv2_osc_forge_bundle(forge, osc, id, buf, size))
		|| (ref = lv2_osc_forge_message(forge, osc, id, buf, size)) )
	{
		return ref;
	}
	return 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV2_OSC_FORGE_H */

/**
   @}
*/
