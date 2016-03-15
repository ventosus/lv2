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

#ifndef LV2_OSC_UTIL_H
#define LV2_OSC_UTIL_H

#include <stdint.h>

#include "lv2/lv2plug.in/ns/ext/osc/osc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
   TODO
*/
static inline bool
lv2_osc_is_packet_type(LV2_OSC *osc, LV2_URID type)
{
	return type == osc->OSC_Packet;
}

/**
   TODO
*/
static inline bool
lv2_osc_is_bundle_type(LV2_OSC *osc, LV2_URID type)
{
	return type == osc->OSC_Bundle;
}

/**
   TODO
*/
static inline bool
lv2_osc_is_message_type(LV2_OSC *osc, LV2_URID type)
{
	return type == osc->OSC_Message;
}

/**
   TODO
*/
static inline bool
lv2_osc_is_osc_type(LV2_OSC *osc, LV2_URID type)
{
	return (type == osc->OSC_Packet)
			|| (type == osc->OSC_Bundle)
			|| (type == osc->OSC_Message);
}

/**
   TODO
*/
static inline bool
lv2_osc_bundle_body_get(LV2_OSC *osc, uint32_t size, const LV2_Atom_Object_Body *body,
	const LV2_OSC_Timestamp **timestamp, const LV2_Atom_Tuple **items)
{
	assert(timestamp && items);

	*timestamp = NULL;
	*items = NULL;

	lv2_atom_object_body_get(size, body,
		osc->OSC_bundleTimestamp, timestamp,
		osc->OSC_bundleItems, items, 
		0);

	if(!*timestamp || ((*timestamp)->atom.type != osc->OSC_Timestamp))
		return false;
	if(!*items || ((*items)->atom.type != osc->ATOM_Tuple))
		return false;

	return true;
}

/**
   TODO
*/
static inline bool
lv2_osc_bundle_get(LV2_OSC *osc, const LV2_Atom_Object *obj,
	const LV2_OSC_Timestamp **timestamp, const LV2_Atom_Tuple **items)
{
	return lv2_osc_bundle_body_get(osc, obj->atom.size, &obj->body,
		timestamp, items);
}

/**
   TODO
*/
static inline bool
lv2_osc_message_body_get(LV2_OSC *osc, uint32_t size, const LV2_Atom_Object_Body *body,
	const LV2_Atom_String **path, const LV2_Atom_Tuple **arguments)
{
	assert(path && arguments);

	*path = NULL;
	*arguments = NULL;

	lv2_atom_object_body_get(size, body,
		osc->OSC_messagePath, path,
		osc->OSC_messageArguments, arguments,
		0);

	if(!*path || ((*path)->atom.type != osc->ATOM_String))
		return false;
	if(arguments && ((*arguments)->atom.type != osc->ATOM_Tuple))
		return false;

	return true;
}

/**
   TODO
*/
static inline bool
lv2_osc_message_get(LV2_OSC *osc, const LV2_Atom_Object *obj,
	const LV2_Atom_String **path, const LV2_Atom_Tuple **arguments)
{
	return lv2_osc_message_body_get(osc, obj->atom.size, &obj->body,
		path, arguments);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV2_OSC_UTIL_H */

/**
   @}
*/
