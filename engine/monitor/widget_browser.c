/*	$Csoft: widget_browser.c,v 1.14 2003/03/16 02:46:56 vedge Exp $	*/

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

#ifdef DEBUG

#include <engine/view.h>

#include <engine/widget/widget.h>
#include <engine/widget/window.h>
#include <engine/widget/text.h>
#include <engine/widget/textbox.h>
#include <engine/widget/tlist.h>
#include <engine/widget/label.h>
#include <engine/widget/button.h>
#include <engine/widget/palette.h>

#include <engine/mapedit/mapview.h>

#include "monitor.h"

static void
tl_windows_poll(int argc, union evarg *argv)
{
	struct tlist *tl = argv[0].p;
	struct window *win;

	tlist_clear_items(tl);

	pthread_mutex_lock(&view->lock);
	TAILQ_FOREACH_REVERSE(win, &view->windows, windows, windowq) {
		tlist_insert_item(tl, NULL, OBJECT(win)->name, win);
	}
	pthread_mutex_unlock(&view->lock);

	tlist_restore_selections(tl);
}

static void
tl_windows_show(int argc, union evarg *argv)
{
	struct tlist *tl = argv[1].p;
	struct tlist_item *it;
	struct window *win;

	it = tlist_item_selected(tl);
	if (it == NULL) {
		text_msg("Error", "No window is selected");
		return;
	}

	win = it->p1;
	window_show(win);
}

static void
tl_windows_hide(int argc, union evarg *argv)
{
	struct tlist *tl = argv[1].p;
	struct tlist_item *it;
	struct window *win;

	it = tlist_item_selected(tl);
	if (it == NULL) {
		text_msg("Error", "No window is selected");
		return;
	}

	win = it->p1;
	window_hide(win);
}

static void
tl_windows_detach(int argc, union evarg *argv)
{
	struct tlist *tl = argv[1].p;
	struct tlist_item *it;
	struct window *win;

	it = tlist_item_selected(tl);
	if (it == NULL) {
		text_msg("Error", "No window is selected");
		return;
	}

	win = it->p1;
	view_detach(win);
}

static void
tl_regions_poll(int argc, union evarg *argv)
{
	struct tlist *tl_regions = argv[0].p;
	struct window *pwin = argv[1].p;
	struct region *reg;

	tlist_clear_items(tl_regions);

	pthread_mutex_lock(&pwin->lock);
	TAILQ_FOREACH(reg, &pwin->regionsh, regions) {
		tlist_insert_item(tl_regions, NULL, OBJECT(reg)->name, reg);
	}
	pthread_mutex_unlock(&pwin->lock);

	tlist_restore_selections(tl_regions);
}

static void
tl_regions_detach(int argc, union evarg *argv)
{
	struct tlist *tl = argv[1].p;
	struct window *win = argv[2].p;
	struct tlist_item *it;
	struct region *reg;

	it = tlist_item_selected(tl);
	if (it == NULL) {
		text_msg("Error", "No region is selected");
		return;
	}

	reg = it->p1;
	window_detach(win, reg);
}

static void
tl_widgets_poll(int argc, union evarg *argv)
{
	struct tlist *tl = argv[0].p;
	struct window *pwin = argv[1].p;
	struct tlist *tl_regions = argv[2].p;
	struct tlist_item *ti;
	struct region *reg;
	struct widget *wid;
	
	tlist_clear_items(tl);

	ti = tlist_item_selected(tl_regions);
	if (ti == NULL) {
		return;
	}
	reg = ti->p1;

	pthread_mutex_lock(&pwin->lock);
	TAILQ_FOREACH(wid, &reg->widgets, widgets) {
		tlist_insert_item(tl, NULL, OBJECT(wid)->name, wid);
	}
	pthread_mutex_unlock(&pwin->lock);

	tlist_restore_selections(tl);
}

static void
tl_widgets_detach(int argc, union evarg *argv)
{
	struct tlist *tl_widgets = argv[1].p;
	struct tlist *tl_regions = argv[2].p;
	struct tlist_item *it;
	struct region *reg;
	struct widget *wid;
	
	it = tlist_item_selected(tl_regions);
	if (it == NULL) {
		text_msg("Error", "No region is selected");
		return;
	}
	reg = it->p1;

	it = tlist_item_selected(tl_widgets);
	if (it == NULL) {
		text_msg("Error", "No widget is selected");
		return;
	}
	wid = it->p1;

	region_detach(reg, wid);
}

static void
tl_colors_poll(int argc, union evarg *argv)
{
	struct tlist *tl = argv[0].p;
	struct widget *wid = argv[1].p;
	struct widget_color *wc;

	tlist_clear_items(tl);

	pthread_mutex_lock(&wid->win->lock);
	SLIST_FOREACH(wc, &wid->colors, colors) {
		tlist_insert_item(tl, NULL, wc->name, wc);
	}
	pthread_mutex_unlock(&wid->win->lock);

	tlist_restore_selections(tl);
}

static void
tl_colors_selected(int argc, union evarg *argv)
{
	struct widget *wid = argv[1].p;
	struct palette *pal = argv[2].p;
	struct tlist_item *it = argv[3].p;
	struct widget_color *color = it->p1;

	widget_bind(pal, "color", WIDGET_UINT32,
	    NULL, &WIDGET_COLOR(wid, color->ind));
}

static void
tl_widgets_examine(int argc, union evarg *argv)
{
	struct tlist *tl = argv[1].p;
	struct tlist_item *it;
	struct widget *wid;
	struct window *win, *pwin;
	struct region *reg;

	it = tlist_item_selected(tl);
	if (it== NULL) {
		text_msg("Error", "No widget is selected.");
		return;
	}
	wid = it->p1;
	pwin = wid->win;

	win = window_generic_new(466, 261,
	    "monitor-widget-browser-%s-%s-wid",
	    OBJECT(pwin)->name, OBJECT(wid)->name);
	if (win == NULL) {
		return;		/* Exists */
	}
	win->minw = 176;
	win->minh = 147;
	window_set_caption(win, "%s widget", OBJECT(wid)->name);
	
	pthread_mutex_lock(&pwin->lock);

	reg = region_new(win, REGION_VALIGN,	0, 0,		100, 50);
	{
		label_new(reg, 100, -1, "Identifier: \"%s\"",
		    OBJECT(wid)->name);

		label_polled_new(reg, 100, -1, &pwin->lock,
		    "Flags: 0x%x", &wid->flags);

		label_polled_new(reg, 100, -1, &pwin->lock,
		    "Type: %s", &wid->type);
		
		label_polled_new(reg, 100, -1, &pwin->lock,
		    "Parent: region %p in window %p", &wid->reg, &wid->win);

		label_polled_new(reg, 100, -1, &pwin->lock,
		    "Geometry: requested %dx%d, allocated %dx%d at [%d,%d]",
		    &wid->rw, &wid->rh, &wid->w, &wid->h, &wid->x, &wid->y);
	}

	/* Color array */
	reg = region_new(win, REGION_VALIGN,	0,  50,		100, 50);
	{
		struct tlist *tl_colors;
		struct palette *pal;

		tl_colors = tlist_new(reg, 100, 50, TLIST_POLL);
		event_new(tl_colors, "tlist-poll",
		    tl_colors_poll, "%p", wid);
	
		pal = palette_new(reg, 100, 50, 3);
		
		event_new(tl_colors, "tlist-changed",
		    tl_colors_selected, "%p, %p", wid, pal);
	}
	
	pthread_mutex_unlock(&pwin->lock);
	window_show(win);
}

static void
tl_windows_examine(int argc, union evarg *argv)
{
	struct tlist *tl = argv[1].p;
	struct tlist_item *it;
	struct window *pwin, *win;
	struct region *reg;
	struct tlist *tl_regions, *tl_widgets;

	it = tlist_item_selected(tl);
	if (it == NULL) {
		text_msg("Error", "No window is selected");
		return;
	}
	pwin = it->p1;

	win = window_generic_new(466, 357,
	    "monitor-widget-browser-%s-win", OBJECT(pwin)->name);
	if (win == NULL) {
		return;		/* Exists */
	}
	window_set_caption(win, "%s window", OBJECT(pwin)->name);
	
	pthread_mutex_lock(&pwin->lock);

	reg = region_new(win, REGION_VALIGN, 0, 0, 100, 45);
	{
		label_new(reg, 100, -1, "Identifier: \"%s\"",
		    OBJECT(pwin)->name);

		label_polled_new(reg, 100, -1, &pwin->lock,
		    "Flags: 0x%x", &pwin->flags);

		label_polled_new(reg, 100, -1, &pwin->lock,
		    "Caption: \"%s\"", &pwin->caption);

		label_polled_new(reg, 100, -1, &pwin->lock,
		    "Effective geometry: %[wxh] at %[x,y]",
		    &pwin->rd, &pwin->rd);

		label_polled_new(reg, 100, -1, &pwin->lock,
		    "Body: %[rect]", &pwin->body);

		label_polled_new(reg, 100, -1, &pwin->lock,
		    "Focus: %p",
		    &pwin->focus);
	}

	/* Region list */
	reg = region_new(win, REGION_HALIGN, 0,  45, 80, 25);
	{
		tl_regions = tlist_new(reg, 100, 100, TLIST_POLL);
		event_new(tl_regions, "tlist-poll",
		    tl_regions_poll, "%p", pwin);
	}
	reg = region_new(win, REGION_VALIGN, 81, 45, 19, 25);
	{
		struct button *bu;

		bu = button_new(reg, "Detach", NULL, 0, 100, 100);
		event_new(bu, "button-pushed",
		    tl_regions_detach, "%p, %p", tl_regions, pwin);
	}

	/* Widget list */
	reg = region_new(win, REGION_HALIGN, 0,  70, 80, 27);
	{
		tl_widgets = tlist_new(reg, 100, 100, TLIST_POLL);
		event_new(tl_widgets, "tlist-poll",
		    tl_widgets_poll, "%p, %p", pwin, tl_regions);
	}
	reg = region_new(win, REGION_VALIGN, 81, 70, 19, 27);
	{
		struct button *bu;

		bu = button_new(reg, "Examine", NULL, 0, 100, 50);
		event_new(bu, "button-pushed",
		    tl_widgets_examine, "%p, %p", tl_widgets);

		bu = button_new(reg, "Detach", NULL, 0, 100, 50);
		event_new(bu, "button-pushed",
		    tl_widgets_detach, "%p, %p, %p", tl_widgets, tl_regions,
		        pwin);
	}

	pthread_mutex_unlock(&pwin->lock);
	window_show(win);
}

struct window *
widget_browser_window(void)
{
	struct window *win;
	struct region *reg;
	struct tlist *tl;


	if ((win = window_generic_new(542, 156, "monitor-widget-browser"))
	    == NULL) {
		return (NULL);	/* Exists */
	}
	window_set_caption(win, "Window stack");

	reg = region_new(win, 0, 0, 0, 80, 100);
	{
		tl = tlist_new(reg, 100, 100, TLIST_POLL);
		event_new(tl, "tlist-poll", tl_windows_poll, NULL);
	}
	
	reg = region_new(win, REGION_VALIGN, 81, 0, 19, 100);
	{
		struct button *bu;
		
		bu = button_new(reg, "Examine", NULL, 0, 100, 25);
		event_new(bu, "button-pushed", tl_windows_examine, "%p", tl);

		bu = button_new(reg, "Show", NULL, 0, 100, 25);
		event_new(bu, "button-pushed", tl_windows_show, "%p", tl);

		bu = button_new(reg, "Hide", NULL, 0, 100, 25);
		event_new(bu, "button-pushed", tl_windows_hide, "%p", tl);

		bu = button_new(reg, "Detach", NULL, 0, 100, 25);
		event_new(bu, "button-pushed", tl_windows_detach, "%p", tl);
	}

	return (win);
}

#endif	/* DEBUG */
