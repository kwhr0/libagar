/*	$Csoft: merge.c,v 1.46 2003/09/07 04:17:37 vedge Exp $	*/

/*
 * Copyright (c) 2002, 2003 CubeSoft Communications, Inc.
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
#include <engine/view.h>

#include "merge.h"

#include <engine/widget/hbox.h>
#include <engine/widget/radio.h>
#include <engine/widget/checkbox.h>
#include <engine/widget/textbox.h>
#include <engine/widget/button.h>
#include <engine/widget/tlist.h>

#include <string.h>

const struct version merge_ver = {
	"agar merge tool",
	6, 0
};

static void	merge_destroy(void *);
static int	merge_load(void *, struct netbuf *);
static int	merge_save(void *, struct netbuf *);
static int	merge_cursor(void *, struct mapview *, SDL_Rect *);
static void	merge_effect(void *, struct mapview *, struct map *,
		             struct node *);
static void	merge_create_brush(int, union evarg *);
static void	merge_edit_brush(int, union evarg *);
static void	merge_remove_brush(int, union evarg *);
static void	merge_load_brush_set(int, union evarg *);
static void	merge_save_brush_set(int, union evarg *);
static void	merge_interpolate(struct merge *, struct map *, struct node *,
		                  struct noderef *, struct map *, struct node *,
				  struct noderef *);

const struct tool_ops merge_ops = {
	{
		NULL,		/* init */
		NULL,		/* reinit */
		merge_destroy,
		merge_load,
		merge_save,
		NULL		/* edit */
	},
	merge_cursor,
	merge_effect,
	NULL			/* mouse */
};

enum {
	NODE, FILL,
	EDNW, EDNO, EDNE,
	EDWE, EDEA,
	EDSW, EDSO, EDSE
};

static const int
merge_table[9][9] = {
       /* nw    n     ne    w     fill  e     sw    s     se */
	{ EDNW, EDNO, EDNO, EDWE, NODE, FILL, FILL, FILL, FILL }, /* nw */
	{ EDNO, EDNO, EDNO, FILL, NODE, FILL, FILL, FILL, FILL }, /* n */
	{ EDNO, EDNO, NODE, 0   , NODE, EDEA, FILL, FILL, EDEA }, /* ne */
	{ EDWE, FILL, FILL, EDWE, NODE, FILL, EDWE, FILL, FILL }, /* w */
	{ FILL, FILL, FILL, FILL, NODE, FILL, FILL, FILL, FILL }, /* fill */
	{ FILL, FILL, EDEA, FILL, NODE, EDEA, FILL, FILL, EDEA }, /* e */
	{ EDWE, 0,    EDEA, EDWE, NODE, 0,    EDSW, EDSO, EDSO }, /* sw */
	{ FILL, FILL, FILL, FILL, NODE, FILL, EDSO, EDSO, EDSO }, /* s */
	{ EDWE, 0,    EDEA, EDNO, NODE, EDNO, EDSO, EDSO, EDSE }  /* se */
};

void
merge_init(void *p)
{
	struct merge *mer = p;
	struct window *win;
	struct hbox *hb;

	tool_init(&mer->tool, "merge", &merge_ops, MAPEDIT_TOOL_MERGE);
	TAILQ_INIT(&mer->brushes);
	
	win = TOOL(mer)->win = window_new("mapedit-tool-merge");
	window_set_caption(win, _("Merge tool"));
	window_set_position(win, WINDOW_MIDDLE_LEFT, 0);
	event_new(win, "window-close", tool_window_close, "%p", mer);

	hb = hbox_new(win, HBOX_WFILL);
	{
		struct button *bu;
		struct textbox *tb;
		
		tb = textbox_new(hb, _("Name: "));
		event_new(tb, "textbox-return", merge_create_brush, "%p, %p",
		    mer, tb);

		bu = button_new(hb, _("Create"));
		button_set_padding(bu, 6);			/* Align */
		button_set_focusable(bu, 0);
		event_new(bu, "button-pushed", merge_create_brush, "%p, %p",
		    mer, tb);
	}

	hb = hbox_new(win, HBOX_HOMOGENOUS|HBOX_WFILL);
	{
		struct button *bu;
		
		bu = button_new(hb, _("Load set"));
		event_new(bu, "button-pushed", merge_load_brush_set, "%p", mer);
		bu = button_new(hb, _("Save set"));
		event_new(bu, "button-pushed", merge_save_brush_set, "%p", mer);
		bu = button_new(hb, _("Edit"));
		event_new(bu, "button-pushed", merge_edit_brush, "%p", mer);
		bu = button_new(hb, _("Remove"));
		event_new(bu, "button-pushed", merge_remove_brush, "%p", mer);
	}
	mer->brushes_tl = tlist_new(win, TLIST_MULTI);
}

static void
merge_free_brushes(struct merge *mer)
{
	struct object *ob, *nob;

	for (ob = TAILQ_FIRST(&mer->brushes);
	     ob != TAILQ_END(&mer->brushes);
	     ob = nob) {
		nob = TAILQ_NEXT(ob, cobjs);
		object_destroy(ob);
		free(ob);
	}
	TAILQ_INIT(&mer->brushes);
}

static void
merge_destroy(void *p)
{
	struct merge *mer = p;

	merge_free_brushes(mer);
	tool_destroy(mer);
}

static void
merge_create_brush(int argc, union evarg *argv)
{
	struct merge *mer = argv[1].p;
	struct textbox *name_tbox = argv[2].p;
	char brush_name[OBJECT_NAME_MAX];
	char m_name[OBJECT_NAME_MAX];
	struct map *m;

	textbox_copy_string(name_tbox, brush_name,
	    sizeof(brush_name) - sizeof("brush()"));
	if (brush_name[0] == '\0') {
		text_msg(MSG_ERROR, _("No brush name was given."));
		return;
	}
	
	snprintf(m_name, sizeof(m_name), "brush(%s)", brush_name);
	if (tlist_item_text(mer->brushes_tl, m_name) != NULL) {
		text_msg(MSG_ERROR, _("A `%s' brush exists."), m_name);
		return;
	}

	m = Malloc(sizeof(struct map));
	map_init(m, m_name);
	if (object_load(m) == -1) {
		if (map_alloc_nodes(m,
		    prop_get_uint32(&mapedit, "default-brush-width"),
		    prop_get_uint32(&mapedit, "default-brush-height")) == -1) {
			text_msg(MSG_ERROR, "map_alloc_nodes: %s", error_get());
			map_destroy(m);
			free(m);
			return;
		}
	}

	TAILQ_INSERT_HEAD(&mer->brushes, OBJECT(m), cobjs);
	tlist_unselect_all(mer->brushes_tl);
	tlist_select(mer->brushes_tl,
	    tlist_insert_item_head(mer->brushes_tl, NULL, m_name, m));
	textbox_printf(name_tbox, " ");
}

static void
merge_edit_brush(int argc, union evarg *argv)
{
	struct merge *mer = argv[1].p;
	struct tlist_item *it;

	TAILQ_FOREACH(it, &mer->brushes_tl->items, items) {
		struct map *brush = it->p1;
		struct window *win;

		if (!it->selected)
			continue;

		win = window_new("mapedit-tool-merge-%s", OBJECT(brush)->name);
		if (win == NULL)
			continue;
		/* XXX close dialog */

		mapview_new(win, brush,
		    MAPVIEW_EDIT|MAPVIEW_GRID|MAPVIEW_PROPS);
		window_show(win);
	}
}

static void
merge_remove_brush(int argc, union evarg *argv)
{
	struct merge *mer = argv[1].p;
	struct tlist_item *it;

	TAILQ_FOREACH(it, &mer->brushes_tl->items, items) {
		if (it->selected) {
			char wname[OBJECT_NAME_MAX];
			struct object *brush = it->p1;
			struct window *win;

			snprintf(wname, sizeof(wname),
			    "win-mapedit-tool-merge-%s", OBJECT(brush)->name);
			if ((win = view_window_exists(wname)) != NULL) {
				window_hide(win);
				view_detach(win);
			}

			TAILQ_REMOVE(&mer->brushes, brush, cobjs);
			tlist_remove_item(mer->brushes_tl, it);
			object_destroy(brush);
			free(brush);
		}
	}
}

static void
merge_load_brush_set(int argc, union evarg *argv)
{
	struct merge *mer = argv[1].p;

	if (object_load(mer) == -1)
		text_msg(MSG_ERROR, "%s", error_get());
}

static void
merge_save_brush_set(int argc, union evarg *argv)
{
	struct merge *mer = argv[1].p;
	
	if (object_save(mer) == -1)
		text_msg(MSG_ERROR, "%s", error_get());
}

static void
merge_interpolate(struct merge *mer, struct map *sm, struct node *sn,
    struct noderef *sr, struct map *dm, struct node *dn, struct noderef *dr)
{
	int sedge = sr->r_gfx.edge;
	int dedge = dr->r_gfx.edge;
	int op = merge_table[sedge][dedge];
	struct noderef *r;
	int x, y;

	dprintf("interp %d <-> %d\n", sedge, dedge);

	switch (op) {
	case NODE:
		node_copy_ref(sr, dm, dn, dm->cur_layer);
		break;
	case FILL:
		for (y = 0; y < sm->maph; y++) {
			for (x = 0; x < sm->mapw; x++) {
				struct node *fn = &sm->map[y][x];
			
				/* XXX randomize/increment */
				TAILQ_FOREACH(r, &fn->nrefs, nrefs) {
					if (r->r_gfx.edge != 0) {
						continue;
					}
					node_copy_ref(r, dm, dn, dm->cur_layer);
				}
				return;
			}
		}
		break;
	case EDNW:
	case EDNO:
	case EDNE:
	case EDWE:
	case EDEA:
	case EDSW:
	case EDSO:
	case EDSE:
		/* Copy the first edge with the requested orientation. */
		for (y = 0; y < sm->maph; y++) {
			for (x = 0; x < sm->mapw; x++) {
				struct node *en = &sm->map[y][x];

				/* XXX randomize/increment */
				TAILQ_FOREACH(r, &en->nrefs, nrefs) {
					if (r->r_gfx.edge != op) {
						continue;
					}
					node_copy_ref(r, dm, dn, dm->cur_layer);
				}
				return;
			}
		}
		break;
	}
}

static void
merge_effect(void *p, struct mapview *mv, struct map *m, struct node *node)
{
	struct merge *mer = p;
	struct tlist_item *it;
	
	/* Avoid circular references. XXX ugly */
	if (strncmp(OBJECT(m)->name, "brush(", 6) == 0)
		return;
	
	TAILQ_FOREACH(it, &mer->brushes_tl->items, items) {
		struct map *sm;
		int sx, sy, dx, dy;

		if (!it->selected)
			continue;

		sm = it->p1;
		for (sy = 0, dy = mv->cy - sm->maph/2;
		     sy < sm->maph && dy < m->maph;
		     sy++, dy++) {
			for (sx = 0, dx = mv->cx - sm->mapw/2;
			     sx < sm->mapw && dx < m->mapw;
			     sx++, dx++) {
				struct node *sn = &sm->map[sy][sx];
				struct node *dn = &m->map[dy][dx];
				struct noderef *sr, *dr;

				TAILQ_FOREACH(sr, &sn->nrefs, nrefs) {
					TAILQ_FOREACH(dr, &dn->nrefs, nrefs) {
						merge_interpolate(mer, sm, sn,
						    sr, m, dn, dr);
					}
				}
			}
		}
	}
}

static int
merge_load(void *p, struct netbuf *buf)
{
	struct merge *mer = p;
	Uint32 i, nbrushes;
	
	if (version_read(buf, &merge_ver, NULL) != 0)
		return (-1);

	merge_free_brushes(mer);
	tlist_clear_items(mer->brushes_tl);

	nbrushes = read_uint32(buf);
	for (i = 0; i < nbrushes; i++) {
		char m_name[OBJECT_NAME_MAX];
		struct map *nbrush;

		copy_string(m_name, buf, sizeof(m_name));
		nbrush = Malloc(sizeof(struct map));
		map_init(nbrush, m_name);
		map_load(nbrush, buf);

		TAILQ_INSERT_HEAD(&mer->brushes, OBJECT(nbrush), cobjs);
		tlist_insert_item_head(mer->brushes_tl, NULL, m_name, nbrush);
	}
	return (0);
}

static int
merge_save(void *p, struct netbuf *buf)
{
	struct merge *mer = p;
	struct object *ob;
	Uint32 nbrushes = 0;
	off_t count_offs;

	version_write(buf, &merge_ver);

	count_offs = netbuf_tell(buf);				/* Skip count */
	write_uint32(buf, 0);	
	TAILQ_FOREACH(ob, &mer->brushes, cobjs) {
		struct brush *brush = (struct brush *)ob;

		write_string(buf, ob->name);
		map_save(brush, buf);
		nbrushes++;
	}
	pwrite_uint32(buf, nbrushes, count_offs);	/* Write count */
	return (0);
}

static int
merge_cursor(void *p, struct mapview *mv, SDL_Rect *rd)
{
	struct merge *mer = p;
	struct noderef *r;
	struct map *sm;
	int sx, sy, dx, dy;
	struct tlist_item *it;
	int rv = -1;
	
	/* XXX ugly work around circular ref */
	if (strncmp(OBJECT(mv->map)->name, "brush(", 6) == 0)
		return (-1);

	TAILQ_FOREACH(it, &mer->brushes_tl->items, items) {
		if (!it->selected)
			continue;
		sm = it->p1;
		for (sy = 0, dy = rd->y - (sm->maph * mv->map->tileh)/2;
		     sy < sm->maph;
		     sy++, dy += mv->map->tileh) {
			for (sx = 0, dx = rd->x - (sm->mapw * mv->map->tilew)/2;
			     sx < sm->mapw;
			     sx++, dx += mv->map->tilew) {
				struct node *sn = &sm->map[sy][sx];

				TAILQ_FOREACH(r, &sn->nrefs, nrefs) {
					noderef_draw(mv->map, r,
					    WIDGET(mv)->cx+dx,
					    WIDGET(mv)->cy+dy);
					rv = 0;
				}
				if (mv->flags & MAPVIEW_PROPS) {
					mapview_draw_props(mv, sn, dx, dy,
					    -1, -1);
				}
			}
		}
	}
	return (rv);
}
