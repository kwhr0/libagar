/*	$Csoft: window.c,v 1.8 2002/04/24 14:08:54 vedge Exp $	*/

/*
 * Copyright (c) 2001, 2002 CubeSoft Communications, Inc.
 * <http://www.csoft.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistribution of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistribution in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of CubeSoft Communications, nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
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

#include <sys/types.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <engine/engine.h>
#include <engine/queue.h>
#include <engine/map.h>
#include <engine/version.h>

#include "widget.h"
#include "window.h"

static const struct obvec window_vec = {
	window_destroy,
	window_load,
	window_save,
	window_link,
	window_unlink
};

/* This list shares world->lock. */
TAILQ_HEAD(, window) windowsh = TAILQ_HEAD_INITIALIZER(windowsh);

static Uint32 delta = 0, delta2 = 256;

/* XXX fucking insane */
void
window_init(struct window *win, struct viewport *view, char *name,
    char *caption, Uint32 flags, Uint32 bgtype, Sint16 x, Sint16 y,
    Uint16 w, Uint16 h)
{
	object_init(&win->obj, name, NULL, 0, &window_vec);

	win->caption = strdup(caption);
	win->view = view;
	win->flags = flags;
	win->bgtype = bgtype;

	switch (win->bgtype) {
	case WINDOW_CUBIC:
	case WINDOW_CUBIC2:
		win->flags |= WINDOW_ANIMATE;
		break;
	}

	win->x = x;
	win->y = y;
	win->w = w;
	win->h = h;
	win->redraw = 0;
	/* XXX pref */
	win->bgcolor = SDL_MapRGBA(view->v->format, 0, 50, 30, 250);
	win->fgcolor = SDL_MapRGBA(view->v->format, 200, 200, 200, 100);

	win->vmask.x = (x / view->map->tilew) - view->mapxoffs;
	win->vmask.y = (y / view->map->tileh) - view->mapyoffs;
	win->vmask.w = (w / view->map->tilew);
	win->vmask.h = (h / view->map->tilew);

	/* XXX pref */
	win->border[0] = SDL_MapRGB(view->v->format, 50, 50, 50);
	win->border[1] = SDL_MapRGB(view->v->format, 100, 100, 160);
	win->border[2] = SDL_MapRGB(view->v->format, 192, 192, 192);
	win->border[3] = SDL_MapRGB(view->v->format, 100, 100, 160);
	win->border[4] = SDL_MapRGB(view->v->format, 50, 50, 50);
	
	TAILQ_INIT(&win->widgetsh);
	win->nwidgets = 0;

	pthread_mutex_init(&win->widgetslock, NULL);
}

void
window_mouse_motion(SDL_Event *ev)
{
}

void
window_key(SDL_Event *ev)
{
}

void
window_mouse_button(SDL_Event *ev)
{
	struct window *win;
	struct widget *w = NULL;

	pthread_mutex_lock(&world->lock);
	TAILQ_FOREACH(win, &windowsh, windows) {
		if (!WINDOW_INSIDE(win, ev->button.x, ev->button.y)) {
			continue;
		}

		dprintf("event to %s\n", OBJECT(win)->name);

		pthread_mutex_lock(&win->widgetslock);
		TAILQ_FOREACH(w, &win->widgetsh, widgets) {
			if (WIDGET_INSIDE(w, ev->button.x, ev->button.y)) {
				widget_event(w, ev, 0);
			}
		}
		pthread_mutex_unlock(&win->widgetslock);

		break;
	}
	pthread_mutex_unlock(&world->lock);
}

void
window_draw(struct window *w)
{
	SDL_Surface *v = w->view->v;
	Uint32 xo, yo, col = 0;
	Uint8 *dst;
	struct widget *wid;

	/* Render the background. */
	if (w->flags & WINDOW_PLAIN) {
		SDL_Rect rd;

		rd.x = w->x;
		rd.y = w->y;
		rd.w = w->w;
		rd.h = w->h;
		
		SDL_FillRect(v, &rd, w->bgcolor);
	} else {
		SDL_LockSurface(v);
		for (yo = 0; yo < w->h; yo++) {
			for (xo = 0; xo < w->w; xo++) {
				dst = (Uint8 *)v->pixels + (w->y + yo) *
				    v->pitch + (w->x+xo) *
				    v->format->BytesPerPixel;

				if (xo > w->w - 4) {
					col = w->border[w->w - xo];
				} else if (yo < 4) {
					col = w->border[yo+1];
				} else if (xo < 4) {
					col = w->border[xo+1];
				} else if (yo > w->h - 4) {
					col = w->border[w->h - yo];
				} else {
					static Uint8 expcom;
			
					switch (w->bgtype) {
					case WINDOW_SOLID:
						col = w->bgcolor;
						break;
					case WINDOW_GRADIENT:
						col = SDL_MapRGBA(v->format,
						    yo >> 2, 0, xo >> 2, 200);
						break;
					case WINDOW_CUBIC:
						expcom =
						    ((yo+delta)^(xo-delta))+
						    delta;
						if (expcom > 150)
							expcom -= 140;
						col = SDL_MapRGBA(v->format,
						    0, expcom, 0, 200);
						break;
					case WINDOW_CUBIC2:
						expcom =
						    ((yo+delta)^(xo-delta))+
						    delta;
						if (expcom > 150) {
							expcom -= 140;
						}
						col = SDL_MapRGBA(v->format,
						    0, 0, expcom, 0);
						col *= (yo*xo);
						break;
					default:
						break;
					}
				}
				switch (v->format->BytesPerPixel) {
				case 1:
					*dst = col;
					break;
				case 2:
					*(Uint16 *)dst = col;
					break;
				case 3:
					if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
						dst[0] = (col>>16) & 0xff;
						dst[1] = (col>>8) & 0xff;
						dst[2] = col & 0xff;
					} else {
						dst[0] = col & 0xff;
						dst[1] = (col>>8) & 0xff;
						dst[2] = (col>>16) & 0xff;
					}
					break;
				case 4:
					*((Uint32 *)dst) = col;
					break;
				}
			}
		}
		SDL_UnlockSurface(v);
	}

	/* Render the widgets. */
	pthread_mutex_lock(&w->widgetslock);
	TAILQ_FOREACH(wid, &w->widgetsh, widgets) {
		widget_draw(wid);
	}
	pthread_mutex_unlock(&w->widgetslock);

	SDL_UpdateRect(v, w->x, w->y, w->w, w->h);

	if (w->flags & WINDOW_ANIMATE) {
		if (delta++ > 256) {
			delta = 0;
		}
		if (delta2-- < 1) {
			delta2 = 256;
		}
		w->redraw++;
	} else {
		w->redraw = 0;
	}
}

int
window_load(void *p, int fd)
{
	struct window *w = (struct window *)p;

	if (version_read(fd, "agar window", 1, 0) != 0) {
		return (-1);
	}
	
	w->flags = fobj_read_uint32(fd);
	w->caption = fobj_read_string(fd);
	w->bgcolor = fobj_read_uint32(fd);
	w->fgcolor = fobj_read_uint32(fd);

	return (0);
}

int
window_save(void *p, int fd)
{
	struct window *w = (struct window *)p;

	version_write(fd, "agar window", 1, 0);

	fobj_write_uint32(fd, w->flags);
	fobj_write_string(fd, w->caption);
	fobj_write_uint32(fd, w->bgcolor);
	fobj_write_uint32(fd, w->fgcolor);

	return (0);
}

int
window_link(void *ob)
{
	struct window *win = (struct window *)ob;

	/* Increment the view mask for this area. */
	view_maskfill(win->view, &win->vmask, 1);

	/* Shares world->lock, which we assume is held. */
	TAILQ_INSERT_HEAD(&windowsh, win, windows);

	win->redraw++;
	
	return (0);
}

int
window_unlink(void *ob)
{
	struct window *win = (struct window *)ob;
	struct widget *wid;

	/* Unlink all widgets implicitely. */
	pthread_mutex_lock(&win->widgetslock);
	TAILQ_FOREACH(wid, &win->widgetsh, widgets) {
		object_unlink(wid);
	}
	pthread_mutex_unlock(&win->widgetslock);

	/* Decrement the view mask for this area. */
	view_maskfill(win->view, &win->vmask, -1);
	if (win->view->map != NULL) {
		win->view->map->redraw++;
	}
	
	return (0);
}

void
window_destroy(void *p)
{
	struct window *win = (struct window *)p;

	free(win->caption);
	pthread_mutex_destroy(&win->widgetslock);
}

