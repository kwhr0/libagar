/*	$Csoft$	*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <SDL.h>
#include <glib.h>
#include <pthread.h>

#include <engine/debug.h>
#include <engine/object.h>
#include <engine/map.h>
#include <engine/view.h>

struct viewport *mainview;

int
view_setmode(struct viewport *v)
{
	v->fps = 30;	/* XXX calibrate */
	v->mapx = 0;
	v->mapy = 0;

	/* XXX make room for edition windows */
	v->mapw = (v->width / v->tilew) - (mapedit ? 1 : 0);
	v->maph = (v->height / v->tileh) - (mapedit ? 1 : 0);

	v->v = SDL_SetVideoMode(v->width, v->height, v->depth, v->flags);
	if (v->v == NULL) {
		fatal("SDL: mode %dx%dx%d: %s\n", v->width, v->height,
		    v->depth, SDL_GetError());
		return (-1);
	}
	SDL_ShowCursor((v->flags & SDL_FULLSCREEN) ? 0: 1);
	return (0);
}

struct viewport *
view_create(int w, int h, int tilew, int tileh, int depth, int flags)
{
	struct viewport *v;

	v = malloc(sizeof(struct viewport));
	if (v == NULL) {
		return (NULL);
	}
	v->width = w;
	v->height = h;
	v->tilew = tilew;
	v->tileh = tileh;
	v->flags = flags;
	v->depth = SDL_VideoModeOK(v->width, v->height, depth, v->flags);
	v->wins = NULL;

	switch (v->depth) {
	case 8:
		v->flags |= SDL_HWPALETTE;
		break;
	}

	if (view_setmode(v) < 0) {
		free(v);
		return (NULL);
	}

	return (v);
}

struct window *
window_create(struct viewport *view, int x, int y, int w, int h, char *caption)
{
	struct window *win;

	win = malloc(sizeof(struct window));
	if (win == NULL) {
		return (NULL);
	}

	win->v = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, view->depth,
	    0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
	if (win->v == NULL) {
		fatal("CreateRGBSurface: %s\n", SDL_GetError());
		free(win);
		return (NULL);
	}

	win->caption = strdup(caption);
	win->width = w;
	win->height = h;
	win->view = view;
	win->x = x;
	win->y = y;

	/* XXX lock */
	view->wins = g_slist_append(view->wins, win);

	return (win);
}

void
window_destroy(void *winp, void *arg)
{
	struct window *win = (struct window *)winp;

	free(win->caption);
	SDL_FreeSurface(win->v);
	free(win);
}

void
view_destroy(struct viewport *v)
{
	/* XXX lock */
	g_slist_foreach(v->wins, (GFunc)window_destroy, NULL);
}

int
view_fullscreen(struct viewport *v, int full)
{
	if (full) {
		v->flags |= SDL_FULLSCREEN;
	} else {
		v->flags &= ~(SDL_FULLSCREEN);
	}
	
	view_setmode(v);
	v->map->redraw++;

	return (0);
}

void
view_center(struct viewport *view, int mapx, int mapy)
{
	view->mapx = mapx - (view->mapw / 2);
	view->mapy = mapy - (view->maph - 2);
	if (view->mapx <= 0)
		view->mapx = 0;
	if (view->mapy <= 0)
		view->mapy = 0;
}

