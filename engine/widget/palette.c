/*	$Csoft: palette.c,v 1.27 2005/01/25 01:18:57 vedge Exp $	*/

/*
 * Copyright (c) 2002, 2003, 2004, 2005 CubeSoft Communications, Inc.
 * <http://www.csoft.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <engine/engine.h>
#include <engine/config.h>
#include <engine/view.h>

#include "palette.h"

#include <engine/widget/primitive.h>
#include <engine/widget/window.h>

const struct widget_ops palette_ops = {
	{
		NULL,		/* init */
		NULL,		/* reinit */
		widget_destroy,
		NULL,		/* load */
		NULL,		/* save */
		NULL		/* edit */
	},
	palette_draw,
	palette_scale
};

static void palette_changed(int, union evarg *);

struct palette *
palette_new(void *parent, enum palette_type type, SDL_PixelFormat *fmt)
{
	struct palette *pal;

	pal = Malloc(sizeof(struct palette), M_OBJECT);
	palette_init(pal, type, fmt);
	object_attach(parent, pal);
	return (pal);
}

void
palette_init(struct palette *pal, enum palette_type type, SDL_PixelFormat *fmt)
{
	int i;

	widget_init(pal, "palette", &palette_ops,
	    WIDGET_FOCUSABLE|WIDGET_WFILL|WIDGET_CLIPPING);
	widget_bind(pal, "color", WIDGET_UINT32, &pal->color);

	pal->color = 0;
	pal->type = type; 
	pal->format = fmt;

	switch (type) {
	case PALETTE_RGB:
		pal->nbars = 3;
		break;
	case PALETTE_RGBA:
		pal->nbars = 4;
		break;
	}

	for (i = 0; i < pal->nbars; i++) {
		pal->bars[i] = scrollbar_new(pal, SCROLLBAR_HORIZ);

		widget_set_int(pal->bars[i], "max", 255);
		event_new(pal->bars[i], "scrollbar-changed", palette_changed,
		    "%p, %i", pal, i);
	}
}

static void
palette_changed(int argc, union evarg *argv)
{
	struct palette *pal = argv[1].p;
	int nbar = argv[2].i;
	struct widget_binding *colorb;
	Uint32 *color;
	Uint8 r, g, b, a;

	colorb = widget_get_binding(pal, "color", &color);
	SDL_GetRGBA(*color, pal->format, &r, &g, &b, &a);
	switch (nbar) {
	case 0:
		r = widget_get_int(pal->bars[nbar], "value");
		break;
	case 1:
		g = widget_get_int(pal->bars[nbar], "value");
		break;
	case 2:
		b = widget_get_int(pal->bars[nbar], "value");
		break;
	case 3:
		a = widget_get_int(pal->bars[nbar], "value");
		break;
	}
	*color = SDL_MapRGBA(pal->format, r, g, b, a);
	widget_binding_unlock(colorb);

	event_post(NULL, pal, "palette-changed", NULL);
}

void
palette_scale(void *p, int w, int h)
{
	struct palette *pal = p;
	int i, y = 0;

	if (w == -1 && h == -1) {
		WIDGET(pal)->w = 128;
		WIDGET(pal)->h = 0;

		for (i = 0; i < pal->nbars; i++) {
			struct widget *bar = (struct widget *)pal->bars[i];

			WIDGET_OPS(bar)->scale(bar, -1, -1);
			if (bar->w > WIDGET(pal)->w) {
				WIDGET(pal)->w = bar->w;
			}
			WIDGET(pal)->h += bar->h;

			bar->x = 0;
			bar->y = y;
			y += bar->h;
		}

		WIDGET(pal)->w += WIDGET(pal)->h;
	} else {
		int sbh = h / pal->nbars;
		int prevw = h;					/* Square */

		for (i = 0; i < pal->nbars; i++) {
			WIDGET(pal->bars[i])->x = 0;
			WIDGET(pal->bars[i])->y = i*sbh;

			widget_scale(pal->bars[i],
			    WIDGET(pal)->w - 8 - prevw,
			    sbh);
		}

		pal->rpreview.x = WIDGET(pal)->w - prevw;
		pal->rpreview.y = 0;
		pal->rpreview.w = prevw;
		pal->rpreview.h = WIDGET(pal)->h;
	}
}

void
palette_draw(void *p)
{
	char text[16];
	struct palette *pal = p;
	Uint32 color, label_color;
	Uint8 r, g, b, a;
	SDL_Surface *label;

	color = widget_get_uint32(pal, "color");
	SDL_GetRGBA(color, pal->format, &r, &g, &b, &a);
	widget_set_int(pal->bars[0], "value", (int)r);
	widget_set_int(pal->bars[1], "value", (int)g);
	widget_set_int(pal->bars[2], "value", (int)b);
	if (pal->nbars >= 4)
		widget_set_int(pal->bars[3], "value", (int)a);

	if (pal->type == PALETTE_RGBA)  {
		int x, y;

		primitives.tiling(pal, pal->rpreview, 16, 0,
		    COLOR(HSVPAL_TILE1_COLOR),
		    COLOR(HSVPAL_TILE2_COLOR));
		for (y = 0; y < WIDGET(pal)->h; y++) {
			for (x = 0; x < WIDGET(pal)->w; x++) {
				view_alpha_blend(view->v,
				    WIDGET(pal)->cx+x,
				    WIDGET(pal)->cy+y,
				    r, g, b, a);
			}
		}
	} else {
		primitives.rect_filled(pal,
		    pal->rpreview.x, pal->rpreview.y,
		    pal->rpreview.w, pal->rpreview.h,
		    color);
	}
	
	label_color = SDL_MapRGB(pal->format, 0, 0, 0);
	snprintf(text, sizeof(text), "%u\n%u\n%u\n%u\n", r, g, b, a);
	label = text_render(prop_get_string(config, "font-engine.default-font"),
	    10, label_color, text);
	widget_blit(pal, label,
	    pal->rpreview.x + pal->rpreview.w/2 - label->w/2,
	    pal->rpreview.y + pal->rpreview.h/2 - label->h/2);
	SDL_FreeSurface(label);
}

