/*
  Copyright 2014 David Robillard <http://drobilla.net>
  Copyright 2014 Hanspeter Portner <http://open-music-kontrollers.ch>

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
   @file osc.h
   C definitions for the LV2 OSC extension <http://lv2plug.in/ns/ext/osc>.
*/

#ifndef LV2_OSC_H
#define LV2_OSC_H

#ifdef __cplusplus
extern "C" {
#endif

#define LV2_OSC_URI                "http://lv2plug.in/ns/ext/osc"
#define LV2_OSC_PREFIX LV2_OSC_URI "#"

#define LV2_OSC__OscEvent          LV2_OSC_PREFIX "OscEvent"

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* LV2_OSC_H */
