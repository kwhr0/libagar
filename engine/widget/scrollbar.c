/*	$Csoft: scrollbar.c,v 1.51 2005/10/01 14:15:39 vedge Exp $	*/

/*
 * Copyright (c) 2002, 2003, 2004, 2005 CubeSoft Communications, Inc.
 * <http://www.csoft.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
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

#include <engine/widget/scrollbar.h>

#include <engine/widget/window.h>
#include <engine/widget/primitive.h>

const AG_WidgetOps agScrollbarOps = {
	{
		NULL,		/* init */
		NULL,		/* reinit */
		AG_WidgetDestroy,
		NULL,		/* load */
		NULL,		/* save */
		NULL		/* edit */
	},
	AG_ScrollbarDraw,
	AG_ScrollbarScale
};

enum ag_button_which {
	AG_BUTTON_NONE,
	AG_BUTTON_UP,
	AG_BUTTON_DOWN,
	AG_BUTTON_SCROLL
};

static void mousebuttonup(int, union evarg *);
static void mousebuttondown(int, union evarg *);
static void mousemotion(int, union evarg *);

AG_Scrollbar *
AG_ScrollbarNew(void *parent, enum ag_scrollbar_type type)
{
	AG_Scrollbar *sb;

	sb = Malloc(sizeof(AG_Scrollbar), M_WIDGET);
	AG_ScrollbarInit(sb, type);
	AG_ObjectAttach(parent, sb);
	return (sb);
}

void
AG_ScrollbarInit(AG_Scrollbar *sb, enum ag_scrollbar_type type)
{
	AG_WidgetInit(sb, "scrollbar", &agScrollbarOps,
	    AG_WIDGET_FOCUSABLE|AG_WIDGET_UNFOCUSED_BUTTONUP);
	AG_WidgetBind(sb, "value", AG_WIDGET_INT, &sb->value);
	AG_WidgetBind(sb, "min", AG_WIDGET_INT, &sb->min);
	AG_WidgetBind(sb, "max", AG_WIDGET_INT, &sb->max);

	sb->value = 0;
	sb->min = 0;
	sb->max = 0;
	sb->type = type;
	sb->curbutton = AG_BUTTON_NONE;
	sb->bw = 15;				/* XXX resolution-dependent */
	sb->barSz = 30;
	sb->arrowSz = 9;

	AG_SetEvent(sb, "window-mousebuttondown", mousebuttondown, NULL);
	AG_SetEvent(sb, "window-mousebuttonup", mousebuttonup, NULL);
	AG_SetEvent(sb, "window-mousemotion", mousemotion, NULL);
}

static void
mousebuttonup(int argc, union evarg *argv)
{
	AG_Scrollbar *sb = argv[0].p;

	sb->curbutton = AG_BUTTON_NONE;
}

/* Clicked or dragged mouse to coord, so adjust value */
static void
AG_ScrollbarMove(AG_Scrollbar *sb, int x, int totalsize)
{
	const int scrolling_area = totalsize - (sb->bw*2);
	int min, max;

	if (sb->curbutton != AG_BUTTON_SCROLL)
		return;
	if (sb->barSz == -1)
		return;
		
	min = AG_WidgetInt(sb, "min");
	max = AG_WidgetInt(sb, "max");
	
	if (max < min)
		return;
	
	/* mouse below min */
	if (x <= sb->bw) {
		AG_WidgetSetInt(sb, "value", min);
	}
	/* mouse above max */
	else if (x >= sb->bw + scrolling_area) {
		AG_WidgetSetInt(sb, "value", max);
	}
	/* mouse between */
	else {
		int nx = x - sb->bw;

		AG_WidgetSetInt(sb, "value", nx*(max-min+1)/scrolling_area);
	}
	
	/* generate an event */
	AG_PostEvent(NULL, sb, "scrollbar-changed", "%i",
		AG_WidgetInt(sb, "value"));
}

static void
mousebuttondown(int argc, union evarg *argv)
{
	AG_Scrollbar *sb = argv[0].p;
	int button = argv[1].i;
	int x = (sb->type == AG_SCROLLBAR_HORIZ) ? argv[2].i : argv[3].i;
	int totalsize = (sb->type == AG_SCROLLBAR_HORIZ) ?
		AGWIDGET(sb)->w : AGWIDGET(sb)->h;
	int min, value, max, nvalue;

	if (button != SDL_BUTTON_LEFT)
		return;

	min = AG_WidgetInt(sb, "min");
	max = AG_WidgetInt(sb, "max");
	value = AG_WidgetInt(sb, "value");

	if (max < min)
		return;
	
	AG_WidgetFocus(sb);
	
	/* click on the up button */
	if (x <= sb->bw) {
		sb->curbutton = AG_BUTTON_UP;
		if (value > min) 
			AG_WidgetSetInt(sb, "value", value - 1);
	}
	/* click on the down button */
	else if (x >= totalsize - sb->bw) {
		sb->curbutton = AG_BUTTON_DOWN;
		if (value < max)
			AG_WidgetSetInt(sb, "value", value + 1);
	}
	/* click in between */
	else {
		sb->curbutton = AG_BUTTON_SCROLL;
		AG_ScrollbarMove(sb, x, totalsize);
	}
	
	/* generate an event if value changed */
	if (value != (nvalue = AG_WidgetInt(sb, "value")))
		AG_PostEvent(NULL, sb, "scrollbar-changed", "%i", nvalue);
}

static void
mousemotion(int argc, union evarg *argv)
{
	AG_Scrollbar *sb = argv[0].p;
	int x = (sb->type == AG_SCROLLBAR_HORIZ) ? argv[1].i : argv[2].i;
	int state = argv[5].i;
	int totalsize = (sb->type == AG_SCROLLBAR_HORIZ) ?
		AGWIDGET(sb)->w : AGWIDGET(sb)->h;

	if (state & SDL_BUTTON_LMASK)
		AG_ScrollbarMove(sb, x, totalsize);
}

void
AG_ScrollbarScale(void *p, int rw, int rh)
{
	AG_Scrollbar *sb = p;

	switch (sb->type) {
	case AG_SCROLLBAR_HORIZ:
		if (rw == -1) {
			AGWIDGET(sb)->w = sb->bw*4;
		}
		if (rh == -1) {
			AGWIDGET(sb)->h = sb->bw;
		} else {
			sb->bw = AGWIDGET(sb)->h;	/* Square */
		}
		break;
	case AG_SCROLLBAR_VERT:
		if (rw == -1) {
			AGWIDGET(sb)->w = sb->bw;
		} else {
			sb->bw = AGWIDGET(sb)->w;	/* Square */
		}
		if (rh == -1) {
			AGWIDGET(sb)->h = sb->bw*4;
		}
		break;
	}
}

void
AG_ScrollbarDraw(void *p)
{
	AG_Scrollbar *sb = p;
	int value, min, max;
	int w, h, x, y;
	int maxcoord;

	if ((max = AG_WidgetInt(sb, "max")) == 0 ||
	     max < (min = AG_WidgetInt(sb, "min"))) {
		return;
	}
	value = AG_WidgetInt(sb, "value");
	
	if (AGWIDGET(sb)->w < sb->bw ||
	    AGWIDGET(sb)->w < sb->bw)
		return;

#ifdef DEBUG
	if (value < min || value > max) {
		dprintf("invalid value: min=%d, value=%d, max=%d\n", min,
		    value, max);
		return;
	}
#endif
	agPrim.box(sb, 0, 0, AGWIDGET(sb)->w, AGWIDGET(sb)->h, -1,
	    AG_COLOR(SCROLLBAR_COLOR));

	switch (sb->type) {
	case AG_SCROLLBAR_VERT:
		if (AGWIDGET(sb)->h < sb->bw*2 + 6) {
			return;
		}
		maxcoord = AGWIDGET(sb)->h - sb->bw*2 - sb->barSz;

		agPrim.box(sb,
		    0, 0,
		    sb->bw, sb->bw, (sb->curbutton == AG_BUTTON_UP) ? -1 : 1,
		    AG_COLOR(SCROLLBAR_BTN_COLOR));
		agPrim.arrow_up(sb, sb->bw/2, sb->bw/2, sb->arrowSz,
		    AG_COLOR(SCROLLBAR_ARR1_COLOR),
		    AG_COLOR(SCROLLBAR_ARR2_COLOR));

		y = AGWIDGET(sb)->h - sb->bw;
		agPrim.box(sb,
		    0, y,
		    sb->bw, sb->bw, (sb->curbutton == AG_BUTTON_DOWN) ? -1 : 1,
		    AG_COLOR(SCROLLBAR_BTN_COLOR));
		agPrim.arrow_down(sb, sb->bw/2, (y + sb->bw/2), sb->arrowSz,
		    AG_COLOR(SCROLLBAR_ARR1_COLOR),
		    AG_COLOR(SCROLLBAR_ARR2_COLOR));
		
		if (sb->barSz == -1) {
			y = 0;
			h = AGWIDGET(sb)->h - sb->bw*2;
		} else {
			y = value * maxcoord / (max-min);
			h = sb->barSz;
			if (sb->bw + y + h >
			    AGWIDGET(sb)->h - sb->bw)
				y = AGWIDGET(sb)->h - sb->bw*2 - h;
		}
		agPrim.box(sb, 0, sb->bw+y, sb->bw, h,
		    (sb->curbutton == AG_BUTTON_SCROLL) ? -1 : 1,
		    AG_COLOR(SCROLLBAR_BTN_COLOR));
		break;
	case AG_SCROLLBAR_HORIZ:
		if (AGWIDGET(sb)->w < sb->bw*2 + 6) {
			return;
		}
		maxcoord = AGWIDGET(sb)->w - sb->bw*2 - sb->barSz;

		agPrim.box(sb,
		    0, 0,
		    sb->bw, sb->bw, (sb->curbutton == AG_BUTTON_UP) ? -1 : 1,
		    AG_COLOR(SCROLLBAR_BTN_COLOR));
		agPrim.arrow_left(sb, sb->bw/2, sb->bw/2, sb->arrowSz,
		    AG_COLOR(SCROLLBAR_ARR1_COLOR),
		    AG_COLOR(SCROLLBAR_ARR2_COLOR));

		x = AGWIDGET(sb)->w - sb->bw;
		agPrim.box(sb,
		    x, 0,
		    sb->bw, sb->bw, (sb->curbutton == AG_BUTTON_DOWN) ? -1 : 1,
		    AG_COLOR(SCROLLBAR_BTN_COLOR));
		agPrim.arrow_right(sb, (x + sb->bw/2), sb->bw/2, sb->arrowSz,
		    AG_COLOR(SCROLLBAR_ARR1_COLOR),
		    AG_COLOR(SCROLLBAR_ARR2_COLOR));
		
		/* Calculate disabled bar */
		if (sb->barSz == -1) {
			x = 0;
			w = AGWIDGET(sb)->w - sb->bw*2;
		} else {
			x = value * maxcoord / (max-min);
			w = sb->barSz;
			if (sb->bw + x + w >
			    AGWIDGET(sb)->w - sb->bw)
				x = AGWIDGET(sb)->w - sb->bw*2 - w;
		}
		agPrim.box(sb, sb->bw+x, 0, w, sb->bw,
		    (sb->curbutton == AG_BUTTON_SCROLL) ? -1 : 1,
		    AG_COLOR(SCROLLBAR_BTN_COLOR));
		break;
	}
#if 0
	{
		SDL_Surface *txt;
		char label[32];

		snprintf(label, sizeof(label), "%d\n%d\n%d\n",
		    value, min, max);
		txt = AG_TextRender(NULL, -1, AG_COLOR(TEXT_COLOR), label);
		AG_WidgetBlit(sb, txt, 0, 0);
		SDL_FreeSurface(txt);
		    
	}
#endif
}

void
AG_ScrollbarSetBarSize(AG_Scrollbar *sb, int bsize)
{
	sb->barSz = (bsize > 10 || bsize == -1) ? bsize : 10;
}

void
AG_ScrollbarGetBarSize(AG_Scrollbar *sb, int *bsize)
{
	*bsize = sb->barSz;
}
