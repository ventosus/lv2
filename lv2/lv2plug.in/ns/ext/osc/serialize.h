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

#ifndef LV2_OSC_SERIALZIE_H
#define LV2_OSC_SERIALZIE_H

#include "lv2/lv2plug.in/ns/ext/osc/forge.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LV2_OSC_PADDED_SIZE(size) ( ( (size_t)(size) + 3 ) & ( ~3 ) )

union _swap32 {
	uint32_t u;
	int32_t i;
	float f;
};

union _swap64 {
	uint64_t u;
	int64_t h;
	double d;
};

static inline LV2_Atom_Forge_Ref
lv2_osc_deserialize_message(LV2_OSC_Forge *oforge, uint32_t id,
	const uint8_t *buf, size_t size)
{
	LV2_Atom_Forge_Frame frame [2];
	LV2_Atom_Forge_Ref ref;

	const void *ptr = buf;
	const void *end = buf + size;

	const char *path = (const char *)ptr;
	if(!lv2_osc_check_path(path))
		return 0;
	ptr += LV2_OSC_PADDED_SIZE(strlen(path)+1);
	if(ptr > end)
		return 0;

	const char *fmt = (const char *)ptr;
	if(!lv2_osc_check_fmt(fmt, 1))
		return 0;
	ptr += LV2_OSC_PADDED_SIZE(strlen(fmt)+1);
	if(ptr > end)
		return 0;

	if(!(ref = lv2_osc_forge_message(oforge, frame, id, path)))
		return 0;

	for(const char *type = fmt+1; *type && (ptr < end); type++)
	{
		switch(*type)
		{
			case LV2_OSC_INT:
			{
				union _swap32 s = { .u = *(const uint32_t *)ptr };
				s.u = be32toh(s.u);
				if(!(ref = lv2_osc_forge_int(oforge, s.i)))
					return 0;
				ptr += 4;
				break;
			}
			case LV2_OSC_FLOAT:
			{
				union _swap32 s = { .u = *(const uint32_t *)ptr };
				s.u = be32toh(s.u);
				if(!(ref = lv2_osc_forge_float(oforge, s.f)))
					return 0;
				ptr += 4;
				break;
			}
			case LV2_OSC_STRING:
			{
				const char *s = ptr;
				const uint32_t len = strlen(s);
				if(!s || !(ref = lv2_osc_forge_string(oforge, s, len)))
					return 0;
				ptr += LV2_OSC_PADDED_SIZE(len+1);
				break;
			}
			case LV2_OSC_SYMBOL:
			{
				const char *s = ptr;
				const uint32_t len = strlen(s);
				const LV2_URID u = 0; //FIXME
				if(!(ref = lv2_osc_forge_symbol(oforge, u)))
					return 0;
				ptr += LV2_OSC_PADDED_SIZE(len+1);
				break;
			}
			case LV2_OSC_BLOB:
			{
				union _swap32 s = { .u = *(const uint32_t *)ptr };
				s.u = be32toh(s.u);
				const int32_t len = s.i;
				const uint8_t *b = ptr + 4;
				if(!(ref = lv2_osc_forge_blob(oforge, b, len)))
					return 0;
				ptr += 4 + LV2_OSC_PADDED_SIZE(len);
				break;
			}
			
			case LV2_OSC_LONG:
			{
				union _swap64 s = { .u = *(const uint64_t *)ptr };
				s.u = be64toh(s.u);
				if(!(ref = lv2_osc_forge_long(oforge, s.h)))
					return 0;
				ptr += 8;
				break;
			}
			case LV2_OSC_DOUBLE:
			{
				union _swap64 s = { .u = *(const uint64_t *)ptr };
				s.u = be64toh(s.u);
				if(!(ref = lv2_osc_forge_double(oforge, s.d)))
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
				if(!(ref = lv2_osc_forge_timestamp(oforge, integral, fraction)))
					return 0;
				ptr += 8;
				break;
			}
			
			case LV2_OSC_MIDI:
			{
				const int32_t len = 3;
				const uint8_t *m = ptr + 1;
				if(!(ref = lv2_osc_forge_midi(oforge, m, len)))
					return 0;
				ptr += 4;
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
lv2_osc_deserialize_packet(LV2_OSC_Forge *oforge, uint32_t id,
	const uint8_t *buf, size_t size);

static inline LV2_Atom_Forge_Ref
lv2_osc_deserialize_bundle(LV2_OSC_Forge *oforge, uint32_t id,
	const uint8_t *buf, size_t size)
{
	LV2_Atom_Forge_Frame frame [2];
	LV2_Atom_Forge_Ref ref;

	const void *ptr = buf;
	const void *end = buf + size;

	if(strncmp(ptr, "#bundle", 8))
		return 0;
	ptr += 8;
	if(ptr > end)
		return 0;

	union _swap64 s64 = { .u = *(const uint64_t *)ptr };
	s64.u = be64toh(s64.u);
	const uint32_t integral = s64.u >> 32;
	const uint32_t fraction = s64.u & 0xffffffff;
	ptr += 8;
	if(ptr > end)
		return 0;

	if(!(ref = lv2_osc_forge_bundle(oforge, frame, id, integral, fraction)))
		return 0;

	while(ptr < end)
	{
		union _swap32 s32 = { .u = *(const uint32_t *)ptr };
		s32.u = be32toh(s32.u);

		if(!(ref = lv2_osc_deserialize_packet(oforge, id, ptr + 4, s32.i)))
			return 0;
		
		ptr += 4 + s32.i;
	}

	lv2_osc_forge_pop(oforge, frame);

	if(ptr == end)
		return ref;

	return 0;
}

static inline LV2_Atom_Forge_Ref
lv2_osc_deserialize_packet(LV2_OSC_Forge *oforge, uint32_t id,
	const uint8_t *buf, size_t size)
{
	LV2_Atom_Forge_Ref ref;

	if(  (ref = lv2_osc_deserialize_bundle(oforge, id, buf, size))
		|| (ref = lv2_osc_deserialize_message(oforge, id, buf, size)) )
		return ref;

	return 0;
}

static inline size_t
lv2_osc_serialize_message(LV2_OSC_Forge *oforge, const LV2_Atom_Object *obj,
	uint8_t *buf, size_t size)
{
	LV2_Atom_Forge *forge = oforge->forge;

	if( (obj->atom.type != forge->Object) || (obj->body.otype != oforge->Message) )
		return 0;

	uint8_t *ptr = buf;
	const uint8_t *end = buf + size;

	const LV2_Atom_String *path = NULL;
	const LV2_Atom_Tuple *args = NULL;

	LV2_Atom_Object_Query q [] = {
		{ oforge->messagePath, (const LV2_Atom **)&path },
		{ oforge->messageArguments, (const LV2_Atom **)&args },
		{ 0, NULL }
	};
	lv2_atom_object_query(obj, q);

	if(!path || (path->atom.type != forge->String))
		return 0;
	if(args && (args->atom.type != forge->Tuple))
		return 0;
	if(size < 8) // smallest possible message
		return 0;

	const size_t path_size = LV2_OSC_PADDED_SIZE(path->atom.size);
	if(ptr + path_size > end)
		return 0;
	strncpy((char *)ptr, LV2_ATOM_BODY_CONST(&path->atom), path_size);
	ptr += path_size;

	unsigned nargs = 0;
	if(args)
	{
		LV2_ATOM_TUPLE_FOREACH(args, arg)
		{
			nargs++;
		}
	}

	const size_t fmt_size = LV2_OSC_PADDED_SIZE(2 + nargs);
	if(ptr + fmt_size > end)
		return 0;
	char *fmt = (char *)ptr;
	strncpy(fmt, ",", fmt_size);
	fmt++; // skip ','
	ptr += fmt_size;

	if(args)
	{
		LV2_ATOM_TUPLE_FOREACH(args, arg)
		{
			if(arg->type == forge->Int)
			{
				if(ptr + 4 > end)
					return 0;
				union _swap32 s32 = { .i = ((const LV2_Atom_Int *)arg)->body };
				s32.u = htobe32(s32.u);
				*(uint32_t *)ptr = s32.u;
				*fmt++ = LV2_OSC_INT;
				ptr += 4;
			}
			else if(arg->type == forge->Float)
			{
				if(ptr + 4 > end)
					return 0;
				union _swap32 s32 = { .f = ((const LV2_Atom_Float *)arg)->body };
				s32.u = htobe32(s32.u);
				*(uint32_t *)ptr = s32.i;
				*fmt++ = LV2_OSC_FLOAT;
				ptr += 4;
			}
			if(arg->type == forge->Long)
			{
				if(ptr + 8 > end)
					return 0;
				union _swap64 s64 = { .h = ((const LV2_Atom_Long *)arg)->body };
				s64.u = htobe64(s64.u);
				*(uint64_t *)ptr = s64.u;
				*fmt++ = LV2_OSC_LONG;
				ptr += 8;
			}
			if(arg->type == forge->Double)
			{
				if(ptr + 8 > end)
					return 0;
				union _swap64 s64 = { .d = ((const LV2_Atom_Double *)arg)->body };
				s64.u = htobe64(s64.u);
				*(uint64_t *)ptr = s64.u;
				*fmt++ = LV2_OSC_DOUBLE;
				ptr += 8;
			}
			if(arg->type == oforge->Timestamp)
			{
				if(ptr + 8 > end)
					return 0;
				const LV2_OSC_Timestamp *timestamp = (const LV2_OSC_Timestamp *)arg;
				union _swap64 s64 = { .u = ((uint64_t)timestamp->body.integral << 32) | timestamp->body.fraction };
				s64.u = htobe64(s64.u);
				*(uint64_t *)ptr = s64.u;
				*fmt++ = LV2_OSC_TIMESTAMP;
				ptr += 8;
			}
			else if(arg->type == forge->Bool)
			{
				*fmt++ = ((const LV2_Atom_Bool *)arg)->body ? LV2_OSC_TRUE : LV2_OSC_FALSE;
			}
			else if(arg->type == 0)
			{
				*fmt++ = LV2_OSC_NIL;
			}
			else if(arg->type == forge->Impulse)
			{
				*fmt++ = LV2_OSC_IMPULSE;
			}
			else if(arg->type == forge->String)
			{
				const size_t str_size = LV2_OSC_PADDED_SIZE(arg->size);
				if(ptr + str_size > end)
					return 0;
				strncpy((char *)ptr, LV2_ATOM_BODY_CONST(arg), str_size);
				*fmt++ = LV2_OSC_STRING;
				ptr += str_size;
			}
			else if(arg->type == forge->URID)
			{
				//FIXME
			}
			else if(arg->type == oforge->MidiEvent)
			{
				if(ptr + 4 > end)
					return 0;
				if(arg->size <= 3)
				{
					ptr[0] = 0x0;
					memcpy(&ptr[1], LV2_ATOM_BODY_CONST(arg), 3);
					*fmt++ = LV2_OSC_MIDI;
					ptr += 4;
				}
			}
			else if(arg->type == forge->Chunk)
			{
				if(ptr + 4 > end)
					return 0;
				union _swap32 s32 = { .u = arg->size };
				s32.u = htobe32(s32.u);
				*(uint32_t *)ptr = s32.u;
				*fmt++ = LV2_OSC_INT;
				ptr += 4;

				const size_t blob_size = LV2_OSC_PADDED_SIZE(s32.u);
				if(ptr + blob_size > end)
					return 0;
				strncpy((char *)ptr, LV2_ATOM_BODY_CONST(arg), blob_size);
				ptr += blob_size;
			}
		}
	}

	return ptr - buf;
}

static inline size_t
lv2_osc_serialize_bundle(LV2_OSC_Forge *oforge, const LV2_Atom_Object *obj,
	uint8_t *buf, size_t size)
{
	LV2_Atom_Forge *forge = oforge->forge;

	if( (obj->atom.type != forge->Object) || (obj->body.otype != oforge->Bundle ) )
		return 0;

	uint8_t *ptr = buf;
	const uint8_t *end = buf + size;

	const LV2_OSC_Timestamp *timestamp = NULL;
	const LV2_Atom_Tuple *items = NULL;

	LV2_Atom_Object_Query q [] = {
		{ oforge->bundleTimestamp, (const LV2_Atom **)&timestamp },
		{ oforge->bundleItems, (const LV2_Atom **)&items },
		{ 0, NULL }
	};
	lv2_atom_object_query(obj, q);

	if(!timestamp || (timestamp->atom.type != oforge->Timestamp))
		return 0;
	if(items && (items->atom.type != forge->Tuple))
		return 0;
	if(size < 28) // smallest possible bundle
		return 0;

	strncpy((char *)ptr, "#bundle", 8);
	ptr += 8;

	union _swap64 s64 = { .u = ((uint64_t)timestamp->body.integral << 32) | timestamp->body.fraction };
	s64.u = htobe64(s64.u);
	*(uint64_t *)ptr = s64.u;
	ptr += 8;

	if(items)
	{
		LV2_ATOM_TUPLE_FOREACH(items, item)
		{
			const LV2_Atom_Object *item_obj = (const LV2_Atom_Object *)item;

			int32_t *item_size = (int32_t *)ptr;
			if(ptr + 4 > end)
				return 0;
			ptr += 4;

			const size_t free_size = end - ptr;
			size_t packet_size;
			if(!((packet_size = lv2_osc_serialize_bundle(oforge, item_obj, ptr, free_size))
				|| (packet_size = lv2_osc_serialize_message(oforge, item_obj, ptr, free_size))))
				return 0;
			ptr += packet_size;

			union _swap32 s32 = { .i = packet_size };
			s32.u = htobe32(s32.u);
			*item_size = s32.i;
		}
	}

	return ptr - buf;
}

static inline size_t
lv2_osc_serialize_packet(LV2_OSC_Forge *oforge, const LV2_Atom_Object *obj,
	uint8_t *buf, size_t size)
{
	size_t packet_size;
	if(!((packet_size = lv2_osc_serialize_bundle(oforge, obj, buf, size))
		|| (packet_size = lv2_osc_serialize_message(oforge, obj, buf, size))))
		return 0;

	return packet_size;
}

/**
   @}
   @}
*/

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* LV2_OSC_SERIALIZE_H */
