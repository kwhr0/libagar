/*	$Csoft: stamp.c,v 1.54 2004/01/03 04:25:10 vedge Exp $	*/

/*
 * Copyright (c) 2002, 2003, 2004 CubeSoft Communications, Inc.
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
#include <engine/mapedit/mapedit.h>

#include <engine/widget/radio.h>

static void stamp_init(void);
static int stamp_cursor(struct mapview *, SDL_Rect *);
static void stamp_effect(struct mapview *, struct map *, struct node *);

struct tool stamp_tool = {
	N_("Stamp"),
	N_("Insert the contents of the copy/paste buffer."),
	MAPEDIT_TOOL_STAMP,
	-1,
	stamp_init,
	NULL,			/* destroy */
	NULL,			/* load */
	NULL,			/* save */
	stamp_effect,
	stamp_cursor,
	NULL			/* mouse */
};

static enum {
	STAMP_REPLACE,		/* Replace other refs on layer */
	STAMP_INSERT_HIGHEST	/* Insert on top of the stack */
} mode = STAMP_REPLACE;

static void
stamp_init(void)
{
	static const char *mode_items[] = {
		N_("Replace"),
		N_("Insert"),
		NULL
	};
	struct window *win;
	struct radio *rad;

	win = tool_window_new(&stamp_tool, "mapedit-tool-stamp");

	rad = radio_new(win, mode_items);
	widget_bind(rad, "value", WIDGET_INT, &mode);
}

static void
stamp_effect(struct mapview *mv, struct map *m, struct node *node)
{
	struct map *copybuf = &mapedit.copybuf;
	int sx, sy, dx, dy;
	
	for (sy = 0, dy = mv->cy - copybuf->maph/2;
	     sy < copybuf->maph && dy < m->maph;
	     sy++, dy++) {
		for (sx = 0, dx = mv->cx - copybuf->mapw/2;
		     sx < copybuf->mapw && dx < m->mapw;
		     sx++, dx++) {
			struct node *sn = &copybuf->map[sy][sx];
			struct node *dn = &m->map[dy][dx];
			struct noderef *r;

			if (mode == STAMP_REPLACE)
				node_clear(m, dn, m->cur_layer);

			TAILQ_FOREACH(r, &sn->nrefs, nrefs)
				node_copy_ref(r, m, dn, m->cur_layer);
		}
	}
}

static int
stamp_cursor(struct mapview *mv, SDL_Rect *rd)
{
	struct map *copybuf = &mapedit.copybuf;
	struct noderef *r;
	int sx, sy, dx, dy;
	int rv = -1;
	
	/* Avoid circular references when viewing the copy buffer. */
	if (mv->map == copybuf)
		return (-1);

	for (sy = 0, dy = rd->y - (copybuf->maph * mv->map->scale)/2;
	     sy < copybuf->maph;
	     sy++, dy += mv->map->scale) {
		for (sx = 0, dx = rd->x - (copybuf->mapw * mv->map->scale)/2;
		     sx < copybuf->mapw;
		     sx++, dx += mv->map->scale) {
			struct node *sn = &copybuf->map[sy][sx];

			TAILQ_FOREACH(r, &sn->nrefs, nrefs) {
				noderef_draw(mv->map, r,
				    WIDGET(mv)->cx+dx,
				    WIDGET(mv)->cy+dy);
				rv = 0;
			}
			if (mv->flags & MAPVIEW_PROPS)
				mapview_draw_props(mv, sn, dx, dy, -1, -1);
		}
	}
	return (rv);
}

