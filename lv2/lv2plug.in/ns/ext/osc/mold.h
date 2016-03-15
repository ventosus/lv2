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
   @defgroup moldialize Mold
   @ingroup osc
   @{
*/

#ifndef LV2_OSC_MOLD_H
#define LV2_OSC_MOLD_H

#include <endian.h>

#include "lv2/lv2plug.in/ns/ext/osc/forge.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef LV2_Atom_Forge_Sink_Handle LV2_OSC_Mold_Sink_Handle;
typedef LV2_Atom_Forge_Ref LV2_OSC_Mold_Ref;
typedef LV2_Atom_Forge_Sink LV2_OSC_Mold_Sink;
typedef LV2_Atom_Forge_Frame LV2_OSC_Mold_Frame;

typedef int32_t *
(*LV2_OSC_Mold_Deref_Func)(LV2_OSC_Mold_Sink_Handle handle,
                             LV2_OSC_Mold_Ref         ref);

/** A "moldialize" for creating atoms by appending to a buffer. */
typedef struct {
	uint8_t *buf;
	uint32_t offset;
	uint32_t size;

	LV2_OSC_Mold_Sink        sink;
	LV2_OSC_Mold_Deref_Func  deref;
	LV2_OSC_Mold_Sink_Handle handle;

	LV2_OSC_Mold_Frame* stack;

	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	LV2_OSC osc;
} LV2_OSC_Mold;

/**
   TODO
*/
static inline void
lv2_osc_mold_set_buffer(LV2_OSC_Mold* mold, uint8_t* buf, size_t size)
{
	mold->buf    = buf;
	mold->size   = (uint32_t)size;
	mold->offset = 0;
	mold->deref  = NULL;
	mold->sink   = NULL;
	mold->handle = NULL;
	mold->stack  = NULL;
}

static inline void
lv2_osc_mold_set_sink(LV2_OSC_Mold*            mold,
                        LV2_OSC_Mold_Sink        sink,
                        LV2_OSC_Mold_Deref_Func  deref,
                        LV2_OSC_Mold_Sink_Handle handle)
{
	mold->buf    = NULL;
	mold->size   = mold->offset = 0;
	mold->deref  = deref;
	mold->sink   = sink;
	mold->handle = handle;
	mold->stack  = NULL;
}

static inline void
lv2_osc_mold_init(LV2_OSC_Mold *mold, LV2_URID_Map *map, LV2_URID_Unmap *unmap)
{
	mold->map = map;
	mold->unmap = unmap;
	lv2_osc_init(&mold->osc, map);
	lv2_osc_mold_set_buffer(mold, NULL, 0);
}

static inline int32_t *
lv2_osc_mold_deref(LV2_OSC_Mold* mold, LV2_OSC_Mold_Ref ref)
{
	if (mold->buf) {
		return (int32_t *)ref;
	} else {
		return mold->deref(mold->handle, ref);
	}
}

static inline LV2_OSC_Mold_Ref
lv2_osc_mold_push(LV2_OSC_Mold*       mold,
                    LV2_OSC_Mold_Frame* frame,
                    LV2_OSC_Mold_Ref    ref)
{
	frame->parent = mold->stack;
	frame->ref    = ref;
	mold->stack  = frame;
	return ref;
}

/** Pop a stack frame.  This must be called when a container is finished. */
static inline void
lv2_osc_mold_pop(LV2_OSC_Mold* mold, LV2_OSC_Mold_Frame* frame)
{
	assert(frame == mold->stack);
	mold->stack = frame->parent;
}

static inline LV2_OSC_Mold_Ref
lv2_osc_mold_raw(LV2_OSC_Mold* mold, const void* data, uint32_t size)
{
	LV2_OSC_Mold_Ref out = 0;
	if (mold->sink) {
		out = mold->sink(mold->handle, data, size);
	} else {
		out = (LV2_OSC_Mold_Ref)mold->buf + mold->offset;
		uint8_t* mem = mold->buf + mold->offset;
		if (mold->offset + size > mold->size) {
			return 0;
		}
		mold->offset += size;
		memcpy(mem, data, size);
	}
	for (LV2_OSC_Mold_Frame* f = mold->stack; f; f = f->parent) {
		int32_t *i = lv2_osc_mold_deref(mold, f->ref);
		union _swap32 s32 = { .i = *i };
		s32.u = be32toh(s32.u);
		s32.i += size;
		s32.u = htobe32(s32.u);
		*i = s32.i;
	}
	return out;
}

static inline void
lv2_osc_mold_pad(LV2_OSC_Mold* mold, uint32_t written)
{
	const uint32_t pad      = 0;
	const uint32_t pad_size = lv2_osc_pad_size(written) - written;
	lv2_osc_mold_raw(mold, &pad, pad_size);
}

/** Write raw output, padding to 32-bits as necessary. */
static inline LV2_OSC_Mold_Ref
lv2_osc_mold_write(LV2_OSC_Mold* mold, const void* data, uint32_t size)
{
	LV2_OSC_Mold_Ref out = lv2_osc_mold_raw(mold, data, size);
	if (out) {
		lv2_osc_mold_pad(mold, size);
	}
	return out;
}

/** Write an atom:Int. */
static inline LV2_OSC_Mold_Ref
lv2_osc_mold_int(LV2_OSC_Mold* mold, int32_t val)
{
	union _swap32 s32 = { .i = val };
	s32.u = htobe32(s32.u);
	return lv2_osc_mold_raw(mold, &s32.u, 4);
}

/** Write an atom:Int. */
static inline LV2_OSC_Mold_Ref
lv2_osc_mold_float(LV2_OSC_Mold* mold, float val)
{
	union _swap32 s32 = { .f = val };
	s32.u = htobe32(s32.u);
	return lv2_osc_mold_raw(mold, &s32.u, 4);
}

/** Write an atom:Int. */
static inline LV2_OSC_Mold_Ref
lv2_osc_mold_long(LV2_OSC_Mold* mold, int64_t val)
{
	union _swap64 s64 = { .h = val };
	s64.u = htobe64(s64.u);
	return lv2_osc_mold_raw(mold, &s64.u, 8);
}

/** Write an atom:Int. */
static inline LV2_OSC_Mold_Ref
lv2_osc_mold_double(LV2_OSC_Mold* mold, double val)
{
	union _swap64 s64 = { .d = val };
	s64.u = htobe64(s64.u);
	return lv2_osc_mold_raw(mold, &s64.u, 8);
}

/** Write an atom:Int. */
static inline LV2_OSC_Mold_Ref
lv2_osc_mold_timestamp(LV2_OSC_Mold* mold, uint32_t integral, uint32_t fraction)
{
	union _swap64 s64 = { .u = ((uint64_t)integral << 32) | fraction };
	s64.u = htobe64(s64.u);
	return lv2_osc_mold_raw(mold, &s64.u, 8);
}

/** Write an atom:Int. */
static inline LV2_OSC_Mold_Ref
lv2_osc_mold_string(LV2_OSC_Mold* mold, const char *val, uint32_t size)
{
	return lv2_osc_mold_write(mold, val, size + 1);
}

/** Write an atom:Int. */
static inline LV2_OSC_Mold_Ref
lv2_osc_mold_symbol(LV2_OSC_Mold* mold, LV2_URID val)
{
	const char *symbol = mold->unmap->unmap(mold->unmap->handle, val);
	return lv2_osc_mold_write(mold, symbol, strlen(symbol) + 1);
}

/** Write an atom:Int. */
static inline LV2_OSC_Mold_Ref
lv2_osc_mold_blob(LV2_OSC_Mold* mold, const uint8_t *val, uint32_t size)
{
	return lv2_osc_mold_write(mold, val, size);
}

/** Write an atom:Int. */
static inline LV2_OSC_Mold_Ref
lv2_osc_mold_midi(LV2_OSC_Mold* mold, uint8_t port, const uint8_t *val, uint32_t size)
{
	LV2_OSC_Mold_Ref ref;
	if(  (ref = lv2_osc_mold_raw(mold, &port, 1))
		&& (ref = lv2_osc_mold_raw(mold, val, 3)) )
		return ref;
	return 0;
}

/** Write an atom:Int. */
static inline LV2_OSC_Mold_Ref
lv2_osc_mold_bundle_head(LV2_OSC_Mold* mold, uint32_t integral, uint32_t fraction)
{
	LV2_OSC_Mold_Ref ref;
	if(  (ref = lv2_osc_mold_raw(mold, "#bundle", 8))
		&& (ref = lv2_osc_mold_timestamp(mold, integral, fraction)) )
		return ref;
	return 0;
}

/** Write an atom:Int. */
static inline LV2_OSC_Mold_Ref
lv2_osc_mold_bundle_item(LV2_OSC_Mold* mold, LV2_OSC_Mold_Frame *frame)
{
	return lv2_osc_mold_push(mold, frame, lv2_osc_mold_int(mold, 0));
}

/** Write an atom:Int. */
static inline LV2_OSC_Mold_Ref
lv2_osc_mold_path(LV2_OSC_Mold* mold, const char *val, uint32_t size)
{
	//TODO check for validity
	return lv2_osc_mold_write(mold, val, size + 1);
}

/** Write an atom:Int. */
static inline LV2_OSC_Mold_Ref
lv2_osc_mold_format(LV2_OSC_Mold* mold, const char *val, uint32_t size)
{
	//TODO check for validity
	return lv2_osc_mold_write(mold, val, size + 1);
}

static LV2_OSC_Mold_Ref
lv2_osc_mold_packet(LV2_OSC_Mold *mold, const LV2_Atom_Object *obj);

static LV2_OSC_Mold_Ref
lv2_osc_mold_message(LV2_OSC_Mold *mold, const LV2_Atom_Object *obj)
{
	LV2_OSC *osc = &mold->osc;

	if(  (obj->atom.type != osc->ATOM_Object)
		|| (obj->body.otype != osc->OSC_Message) )
		return 0;

	const LV2_Atom_String *path = NULL;
	const LV2_Atom_Tuple *args = NULL;
	if(!lv2_osc_message_get(osc, obj, &path, &args))
		return 0;

	LV2_OSC_Mold_Ref ref;
	if(!(ref = lv2_osc_mold_path(mold, LV2_ATOM_BODY_CONST(path), path->atom.size)))
		return 0;

	const uint8_t fmt [] = {',', '\0'};
	if(!(ref = lv2_osc_mold_raw(mold, &fmt[0], 1)))
		return 0;

	unsigned nargs = 0;
	if(args)
	{
		LV2_ATOM_TUPLE_FOREACH(args, arg)
		{
			nargs++;

			uint8_t type;
			if(arg->type == osc->ATOM_Int)
				type = LV2_OSC_INT;
			else if(arg->type == osc->ATOM_Float)
				type = LV2_OSC_FLOAT;
			else if(arg->type == osc->ATOM_String)
				type = LV2_OSC_STRING;
			else if(arg->type == osc->ATOM_Chunk)
				type = LV2_OSC_BLOB;
			else if(arg->type == osc->ATOM_Bool)
				type = ((const LV2_Atom_Bool *)arg)->body ? LV2_OSC_TRUE : LV2_OSC_FALSE;
			else if( (arg->type == 0) && (arg->size == 0) )
				type = LV2_OSC_NIL;
			else if(arg->type == osc->ATOM_Impulse)
				type = LV2_OSC_IMPULSE;
			else if(arg->type == osc->ATOM_Long)
				type = LV2_OSC_LONG;
			else if(arg->type == osc->ATOM_Double)
				type = LV2_OSC_DOUBLE;
			else if(arg->type == osc->OSC_Timestamp)
				type = LV2_OSC_TIMESTAMP;
			else if(arg->type == osc->MIDI_MidiEvent)
				type = LV2_OSC_MIDI;
			else if(arg->type == osc->ATOM_URID)
				type = LV2_OSC_SYMBOL;
			else
				return 0;

			if(!(ref = lv2_osc_mold_raw(mold, &type, 1)))
				return 0;
		}
		if(!(ref = lv2_osc_mold_raw(mold, &fmt[1], 1)))
			return 0;

		lv2_osc_mold_pad(mold, nargs + 2); // ',' '\0'

		LV2_ATOM_TUPLE_FOREACH(args, arg)
		{
			if(arg->type == osc->ATOM_Int)
				ref = lv2_osc_mold_int(mold, ((const LV2_Atom_Int *)arg)->body);
			else if(arg->type == osc->ATOM_Float)
				ref = lv2_osc_mold_float(mold, ((const LV2_Atom_Float *)arg)->body);
			else if(arg->type == osc->ATOM_String)
				ref = lv2_osc_mold_string(mold, LV2_ATOM_BODY_CONST(arg), arg->size);
			else if(arg->type == osc->ATOM_Chunk)
				ref = lv2_osc_mold_blob(mold, LV2_ATOM_BODY_CONST(arg), arg->size);
			else if(arg->type == osc->ATOM_Long)
				ref = lv2_osc_mold_long(mold, ((const LV2_Atom_Long *)arg)->body);
			else if(arg->type == osc->ATOM_Double)
				ref = lv2_osc_mold_double(mold, ((const LV2_Atom_Double *)arg)->body);
			else if(arg->type == osc->OSC_Timestamp)
				ref = lv2_osc_mold_timestamp(mold, ((const LV2_OSC_Timestamp *)arg)->body.integral, ((const LV2_OSC_Timestamp *)arg)->body.fraction);
			else if(arg->type == osc->MIDI_MidiEvent)
				ref = lv2_osc_mold_midi(mold, 0x0, LV2_ATOM_BODY_CONST(arg), arg->size);
			else if(arg->type == osc->ATOM_URID)
				ref = lv2_osc_mold_symbol(mold, ((const LV2_Atom_URID *)arg)->body);
			else
				return 0;

			if(!ref)
				return 0;
		}
	}

	return ref;
}

static LV2_OSC_Mold_Ref
lv2_osc_mold_bundle(LV2_OSC_Mold *mold, const LV2_Atom_Object *obj)
{
	LV2_OSC *osc = &mold->osc;

	if(  (obj->atom.type != osc->ATOM_Object)
		|| (obj->body.otype != osc->OSC_Bundle) )
		return 0;

	const LV2_OSC_Timestamp *timestamp = NULL;
	const LV2_Atom_Tuple *items = NULL;
	if(!lv2_osc_bundle_get(osc, obj, &timestamp, &items))
		return 0;

	LV2_OSC_Mold_Ref ref;
	if(!(ref = lv2_osc_mold_bundle_head(mold, timestamp->body.integral, timestamp->body.fraction)))
		return 0;

	if(items) //TODO are empty bundles valid?
	{
		LV2_ATOM_TUPLE_FOREACH(items, item)
		{
			LV2_OSC_Mold_Frame frame;
			const LV2_Atom_Object *item_obj = (const LV2_Atom_Object *)item;

			if(  (ref = lv2_osc_mold_bundle_item(mold, &frame))
				&& (ref = lv2_osc_mold_packet(mold, item_obj)) )
				lv2_osc_mold_pop(mold, &frame);
			else
				return 0;
		}
	}

	return ref;
}

static LV2_OSC_Mold_Ref
lv2_osc_mold_packet(LV2_OSC_Mold *mold, const LV2_Atom_Object *obj)
{
	LV2_OSC_Mold_Ref ref;
	if((ref = lv2_osc_mold_bundle(mold, obj)))
		return ref;
	return lv2_osc_mold_message(mold, obj);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV2_OSC_EMBOSS_H */

/**
   @}
*/
