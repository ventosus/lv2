/*
  LV2 Parameter Example Plugin
  Copyright 2014-2015 David Robillard <d@drobilla.net>

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/log/logger.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/osc/forge.h"
#include "lv2/lv2plug.in/ns/ext/osc/serialize.h"

#define EG_OSC_URI "http://lv2plug.in/plugins/eg-osc"
#define BUF_SIZE 2048

enum {
	OSC_IN  = 0,
	OSC_OUT = 1,
	TOGGLE  = 2
};

typedef struct {
	// Features
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	LV2_Log_Log *log;

	// Forge for creating atoms
	LV2_Atom_Forge forge;

	// Forge for creating OSC atoms
	LV2_OSC_Forge oforge;

	// Logger convenience API
	LV2_Log_Logger logger;

	// Ports
	const LV2_Atom_Sequence *event_in;
	LV2_Atom_Sequence *event_out;
	const float *toggle;
	bool _toggle;

	uint8_t buf [BUF_SIZE];
} Plugin;

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Plugin *self = instance;

	switch(port)
	{
		case OSC_IN:
			self->event_in = data;
			break;
		case OSC_OUT:
			self->event_out = data;
			break;
		case TOGGLE:
			self->toggle = data;
			break;
		default:
			break;
	}
}

static inline int
get_features(const LV2_Feature* const* features, ...)
{
	va_list args;
	va_start(args, features);

	const char* uri = NULL;
	while ((uri = va_arg(args, const char*))) {
		void** data     = va_arg(args, void**);
		bool   required = va_arg(args, int);
		bool   found    = false;

		for (int i = 0; features[i]; ++i) {
			if (!strcmp(features[i]->URI, uri)) {
				*data = features[i]->data;
				found = true;
				break;
			}
		}

		if (required && !found) {
			fprintf(stderr, "Missing required feature <%s>\n", uri);
			return 1;
		}
	}

	return 0;
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               path,
            const LV2_Feature* const* features)
{
	// Allocate instance
	Plugin *self = calloc(1, sizeof(Plugin));
	if(!self)
		return NULL;

	// Get host features
	if (get_features(features,
	                 LV2_URID__map,   &self->map,   true,
	                 LV2_URID__unmap, &self->unmap, true,
	                 LV2_LOG__log,    &self->log,   true,
	                 NULL)) {
		free(self);
		return NULL;
	}

	// Map URIs and initialise forge/logger
	lv2_atom_forge_init(&self->forge, self->map);
	lv2_osc_forge_init(&self->oforge, self->map);
	lv2_log_logger_init(&self->logger, self->map, self->log);

	return self;
}

static void
cleanup(LV2_Handle instance)
{
	Plugin *self = instance;

	free(self);
}

static inline void
_dump_message(Plugin *self, int64_t frames, const LV2_Atom_Object *obj, int indent)
{
	LV2_OSC_Forge *oforge = &self->oforge;

	const LV2_Atom_String *path;
	const LV2_Atom_Tuple *arguments;
	if(lv2_osc_forge_message_unpack(oforge, obj, &path, &arguments))
	{
		const char *path_str = LV2_ATOM_BODY_CONST(path);
		lv2_log_trace(&self->logger, "%*s", indent + strlen(path_str), path_str);

		LV2_ATOM_TUPLE_FOREACH(arguments, argument)
		{
			const char *type_str = self->unmap->unmap(self->unmap->handle, argument->type);
			lv2_log_trace(&self->logger, "%*s", 2 + indent + strlen(type_str), type_str);
		}
	}
}

static inline void
_dump_bundle(Plugin *self, int64_t frames, const LV2_Atom_Object *obj, int indent)
{
	LV2_OSC_Forge *oforge = &self->oforge;

	const LV2_OSC_Timestamp *timestamp;
	const LV2_Atom_Tuple *items;
	if(lv2_osc_forge_bundle_unpack(oforge, obj, &timestamp, &items))
	{
		lv2_log_trace(&self->logger, "%*s%08x.%08x", indent, "", timestamp->body.integral, timestamp->body.fraction);

		LV2_ATOM_TUPLE_FOREACH(items, item)
		{
			const LV2_Atom_Object *item_obj = (const LV2_Atom_Object *)item;

			if(lv2_osc_forge_is_bundle_type(oforge, item_obj->body.otype))
				_dump_bundle(self, frames, item_obj, indent + 2);
			else if(lv2_osc_forge_is_message_type(oforge, item_obj->body.otype))
				_dump_message(self, frames, item_obj, indent + 2);
		}
	}
}

static void
run(LV2_Handle instance, uint32_t sample_count)
{
	Plugin *self = instance;
	LV2_Atom_Forge *forge = &self->forge;
	LV2_OSC_Forge *oforge = &self->oforge;

	const bool _toggle = floor(*self->toggle);

	const uint32_t capacity = self->event_out->atom.size;
	lv2_atom_forge_set_buffer(forge, (uint8_t *)self->event_out, capacity);
	lv2_osc_forge_set_forge(oforge, forge);

	LV2_Atom_Forge_Frame frame;
	LV2_Atom_Forge_Frame bundle_frame [2];
	LV2_Atom_Forge_Ref ref = lv2_atom_forge_sequence_head(forge, &frame, 0);

	if(_toggle != self->_toggle)
	{
		self->_toggle = _toggle;
		if(ref)
			ref = lv2_atom_forge_frame_time(forge, 0);
		if(ref)
			ref = lv2_osc_forge_bundle(oforge, bundle_frame, 0, 0, 1);
		if(ref)
			ref = lv2_osc_forge_message_vararg(oforge, 0, "/LV2", "is", 2016, "rocks");
		if(ref)
			lv2_osc_forge_pop(oforge, bundle_frame);
	}

	LV2_ATOM_SEQUENCE_FOREACH(self->event_in, ev)
	{
		if(lv2_osc_forge_is_packet_type(oforge, ev->body.type))
		{
			lv2_log_trace(&self->logger, "OSC packet");
			const uint8_t *buf = LV2_ATOM_BODY_CONST(&ev->body);
			const size_t size = ev->body.size;

			if(ref)
				ref = lv2_atom_forge_frame_time(forge, ev->time.frames);
			if(ref)
				ref = lv2_osc_deserialize_packet(oforge, 0, buf, size);
		}
		else if(lv2_atom_forge_is_object_type(forge, ev->body.type))
		{
			const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;

			if(lv2_osc_forge_is_bundle_type(oforge, obj->body.otype))
			{
				lv2_log_trace(&self->logger, "OSC bundle");
				_dump_bundle(self, ev->time.frames, obj, 0);

				const size_t size = lv2_osc_serialize_bundle(oforge, obj, self->buf, BUF_SIZE);
				if(size)
				{
					if(ref)
						ref = lv2_atom_forge_frame_time(forge, ev->time.frames);
					if(ref)
						ref = lv2_atom_forge_atom(forge, size, oforge->Packet);
					if(ref)
						ref = lv2_atom_forge_raw(forge, self->buf, size);
					if(ref)
						lv2_atom_forge_pad(forge, size);
				}
			}
			else if(lv2_osc_forge_is_message_type(oforge, obj->body.otype))
			{
				lv2_log_trace(&self->logger, "OSC message");
				_dump_message(self, ev->time.frames, obj, 0);

				const size_t size = lv2_osc_serialize_message(oforge, obj, self->buf, BUF_SIZE);
				if(size)
				{
					if(ref)
						ref = lv2_atom_forge_frame_time(forge, ev->time.frames);
					if(ref)
						ref = lv2_atom_forge_atom(forge, size, oforge->Packet);
					if(ref)
						ref = lv2_atom_forge_raw(forge, self->buf, size);
					if(ref)
						lv2_atom_forge_pad(forge, size);
				}
			}
		}
	}

	if(ref)
		lv2_atom_forge_pop(forge, &frame);
	else
		lv2_atom_sequence_clear(self->event_out);
}

static const LV2_Descriptor descriptor = {
	EG_OSC_URI,
	instantiate,
	connect_port,
	NULL,  // activate,
	run,
	NULL,  // deactivate,
	cleanup,
	NULL // extension_data
};

LV2_SYMBOL_EXPORT const LV2_Descriptor *
lv2_descriptor(uint32_t index)
{
	return (index == 0) ? &descriptor : NULL;
}
