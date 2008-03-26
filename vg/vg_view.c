/*
 * Copyright (c) 2005-2008 Hypertriton, Inc. <http://hypertriton.com/>
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

/*
 * Visualization widget.
 */

#include <core/core.h>
#include <core/config.h>

#include <stdarg.h>
#include <string.h>

#include "vg.h"
#include "vg_view.h"

#include <gui/view.h>
#include <gui/window.h>
#include <gui/primitive.h>

VG_View *
VG_ViewNew(void *parent, VG *vg, Uint flags)
{
	VG_View *vv;

	vv = Malloc(sizeof(VG_View));
	AG_ObjectInit(vv, &vgViewClass);
	vv->flags |= flags;
	vv->vg = vg;
	
	if (flags & VG_VIEW_HFILL) { AG_ExpandHoriz(vv); }
	if (flags & VG_VIEW_VFILL) { AG_ExpandVert(vv); }

	AG_ObjectAttach(parent, vv);
	return (vv);
}

static void
MouseMotion(AG_Event *event)
{
	VG_View *vv = AG_SELF();
	VG_Tool *tool = VG_CURTOOL(vv);
	int xCurs = AG_INT(1);
	int yCurs = AG_INT(2);
	float xRel = (float)AG_INT(3);
	float yRel = (float)AG_INT(4);
	int state = AG_INT(5);
	float x, y, xCt, yCt;

	VG_GetVGCoords(vv, xCurs,yCurs, &x,&y);
	xCt = x;
	yCt = y;

	if (vv->mouse.panning) {
		vv->x += xRel;
		vv->y += yRel;
		return;
	}
	if (tool != NULL && tool->ops->mousemotion != NULL) {
		if (!(tool->ops->flags & VG_MOUSEMOTION_NOSNAP)) {
			VG_ApplyConstraints(vv, &xCt,&yCt);
		}
		if (tool->ops->mousemotion(tool, xCt, yCt,
		    (int)(xRel*vv->scale),
		    (int)(yRel*vv->scale), state) == 1) {
			vv->mouse.x = x;
			vv->mouse.y = y;
			return;
		}
	} else {
		vv->mouse.x = x;
		vv->mouse.y = y;
	}
}

static void
MouseButtonDown(AG_Event *event)
{
	VG_View *vv = AG_SELF();
	VG_Tool *tool = VG_CURTOOL(vv);
	int button = AG_INT(1);
	int xCurs = AG_INT(2);
	int yCurs = AG_INT(3);
	VG_ToolMouseBinding *mb;
	float x, y, xCt, yCt;

	VG_GetVGCoords(vv, xCurs,yCurs, &x,&y);
	xCt = x;
	yCt = y;

	AG_WidgetFocus(vv);
	vv->mouse.x = x;
	vv->mouse.y = y;
	switch (button) {
	case SDL_BUTTON_MIDDLE:
		vv->mouse.panning = 1;
		break;
	case SDL_BUTTON_WHEELDOWN:
		VG_ViewSetScale(vv, vv->scale-1.5f);
		return;
	case SDL_BUTTON_WHEELUP:
		VG_ViewSetScale(vv, vv->scale+1.5f);
		return;
	default:
		break;
	}
	if (tool != NULL && tool->ops->mousebuttondown != NULL) {
		if (!(tool->ops->flags & VG_BUTTONDOWN_NOSNAP)) {
			VG_ApplyConstraints(vv, &xCt,&yCt);
		}
		if (tool->ops->mousebuttondown(tool, xCt,yCt, button) == 1)
			return;
	}
	TAILQ_FOREACH(tool, &vv->tools, tools) {
		SLIST_FOREACH(mb, &tool->mbindings, mbindings) {
			if (mb->button != button) {
				continue;
			}
			tool->vgv = vv;
			if (mb->func(tool, button, 1, x, y, mb->arg) == 1)
				return;
		}
	}
	if (vv->btndown_ev != NULL)
		AG_PostEvent(NULL, vv, vv->btndown_ev->name,
		    "%i,%f,%f", button, x, y);
}

static void
MouseButtonUp(AG_Event *event)
{
	VG_View *vv = AG_SELF();
	VG_Tool *tool = VG_CURTOOL(vv);
	int button = AG_INT(1);
	int xCurs = AG_INT(2);
	int yCurs = AG_INT(3);
	VG_ToolMouseBinding *mb;
	float x, y, xCt, yCt;
	
	VG_GetVGCoords(vv, xCurs,yCurs, &x,&y);
	xCt = x;
	yCt = y;

	if (tool != NULL && tool->ops->mousebuttonup != NULL) {
		if (!(tool->ops->flags & VG_BUTTONUP_NOSNAP)) {
			VG_ApplyConstraints(vv, &xCt,&yCt);
		}
		if (tool->ops->mousebuttonup(tool, xCt,yCt, button) == 1)
			return;
	}
	TAILQ_FOREACH(tool, &vv->tools, tools) {
		SLIST_FOREACH(mb, &tool->mbindings, mbindings) {
			if (mb->button != button) {
				continue;
			}
			tool->vgv = vv;
			if (mb->func(tool, button, 0, x, y, mb->arg) == 1)
				return;
		}
	}
	switch (button) {
	case SDL_BUTTON_MIDDLE:
		vv->mouse.panning = 0;
		return;
	default:
		break;
	}
	if (vv->btnup_ev != NULL)
		AG_PostEvent(NULL, vv, vv->btnup_ev->name, "%i,%f,%f",
		    button, x, y);
}

static void
Init(void *obj)
{
	VG_View *vv = obj;

	WIDGET(vv)->flags |= AG_WIDGET_FOCUSABLE|AG_WIDGET_CLIPPING;

	vv->flags = 0;
	vv->vg = NULL;
	vv->draw_ev = NULL;
	vv->scale_ev = NULL;
	vv->keydown_ev = NULL;
	vv->btndown_ev = NULL;
	vv->keyup_ev = NULL;
	vv->btnup_ev = NULL;
	vv->motion_ev = NULL;
	vv->x = 0;
	vv->y = 0;
	vv->scale = 1.0f;
	vv->wPixel = 1.0f;
	vv->snap_mode = VG_GRID;
	vv->ortho_mode = VG_NO_ORTHO;
	vv->gridIval = 4.0f;
	vv->mouse.x = 0.0f;
	vv->mouse.y = 0.0f;
	vv->mouse.panning = 0;
	vv->curtool = NULL;
	vv->deftool = NULL;
	vv->status[0] = '\0';
	TAILQ_INIT(&vv->tools);

	AG_SetEvent(vv, "window-mousemotion", MouseMotion, NULL);
	AG_SetEvent(vv, "window-mousebuttondown", MouseButtonDown, NULL);
	AG_SetEvent(vv, "window-mousebuttonup", MouseButtonUp, NULL);
}

void
VG_ViewSetSnapMode(VG_View *vv, enum vg_snap_mode mode)
{
	vv->snap_mode = mode;
}

void
VG_ViewSetOrthoMode(VG_View *vv, enum vg_ortho_mode mode)
{
	vv->ortho_mode = mode;
}

void
VG_ViewSetGridInterval(VG_View *vv, float ival)
{
	vv->gridIval = MAX(1.0f,ival);
}

void
VG_ViewDrawFn(VG_View *vv, AG_EventFn fn, const char *fmt, ...)
{
	vv->draw_ev = AG_SetEvent(vv, NULL, fn, NULL);
	AG_EVENT_GET_ARGS(vv->draw_ev, fmt);
}

void
VG_ViewScaleFn(VG_View *vv, AG_EventFn fn, const char *fmt, ...)
{
	vv->scale_ev = AG_SetEvent(vv, NULL, fn, NULL);
	AG_EVENT_GET_ARGS(vv->scale_ev, fmt);
}

void
VG_ViewKeydownFn(VG_View *vv, AG_EventFn fn, const char *fmt, ...)
{
	vv->keydown_ev = AG_SetEvent(vv, "window-keydown", fn, NULL);
	AG_EVENT_GET_ARGS(vv->keydown_ev, fmt);
}

void
VG_ViewKeyupFn(VG_View *vv, AG_EventFn fn, const char *fmt, ...)
{
	vv->keyup_ev = AG_SetEvent(vv, "window-keyup", fn, NULL);
	AG_EVENT_GET_ARGS(vv->keyup_ev, fmt);
}

void
VG_ViewButtondownFn(VG_View *vv, AG_EventFn fn, const char *fmt, ...)
{
	vv->btndown_ev = AG_SetEvent(vv, NULL, fn, NULL);
	AG_EVENT_GET_ARGS(vv->btndown_ev, fmt);
}

void
VG_ViewButtonupFn(VG_View *vv, AG_EventFn fn, const char *fmt, ...)
{
	vv->btnup_ev = AG_SetEvent(vv, "window-mousebuttonup", fn, NULL);
	AG_EVENT_GET_ARGS(vv->btnup_ev, fmt);
}

void
VG_ViewMotionFn(VG_View *vv, AG_EventFn fn, const char *fmt, ...)
{
	vv->motion_ev = AG_SetEvent(vv, "window-mousemotion", fn, NULL);
	AG_EVENT_GET_ARGS(vv->motion_ev, fmt);
}

static void
SizeRequest(void *p, AG_SizeReq *r)
{
	r->w = 16;
	r->h = 16;
}

static int
SizeAllocate(void *p, const AG_SizeAlloc *a)
{
	if (a->w < 16 || a->h < 16) {
		return (-1);
	}
	return (0);
}

static __inline__ void
DrawGrid(VG_View *vv)
{
	int x, y, ival;
	Uint32 c32;

	ival = vv->gridIval*vv->scale;
	if (ival < 4)
		return;

	c32 = VG_MapColorRGB(vv->vg->gridColor);
	for (y = vv->y%(ival-1); y < HEIGHT(vv); y += ival) {
		for (x = vv->x%(ival-1); x < WIDTH(vv); x += ival)
			AG_DrawPixel(vv, x, y, c32);
	}
}

static void
Draw(void *p)
{
	VG_View *vv = p;
	VG *vg = vv->vg;
	SDL_Surface *status;
	VG_Color colorSave;
	VG_Element *vge;

	AG_DrawRectFilled(vv, AG_RECT(0,0,WIDTH(vv),HEIGHT(vv)),
	    VG_MapColorRGB(vg->fillColor));

	AG_MutexLock(&vg->lock);
	
	if (vv->draw_ev != NULL)
		vv->draw_ev->handler(vv->draw_ev);

	if (vg->flags & VG_VISGRID)
		DrawGrid(vv);
	if (vg->flags & VG_VISORIGIN)
		VG_DrawOrigin(vv);
#ifdef DEBUG
	if (vg->flags & VG_VISBBOXES)
		VG_DrawExtents(vv);
#endif
	TAILQ_FOREACH(vge, &vg->vges, vges) {
		colorSave = vge->color;

		if (vge->mouseover) {
			/* XXX */
			if (vge->color.r > 200 &&
			    vge->color.g > 200 &&
			    vge->color.b > 200) {
				vge->color.r = 0;
				vge->color.g = 255;
				vge->color.b = 0;
			} else {
				vge->color.r = MIN(vge->color.r+50,255);
				vge->color.g = MIN(vge->color.g+50,255);
				vge->color.b = MIN(vge->color.b+50,255);
			}
		}

		vge->ops->draw(vv, vge);

		if (vge->mouseover)
			vge->color = colorSave;
	}

	AG_MutexUnlock(&vg->lock);

	/* XXX */
	AG_TextColor(TEXT_COLOR);
	status = AG_TextRender(vv->status);
	AG_WidgetBlit(vv, status, 0, WIDGET(vv)->h - status->h);
	SDL_FreeSurface(status);
}

void
VG_ViewSelectTool(VG_View *vv, VG_Tool *ntool, void *p)
{
	if (vv->curtool != NULL) {
		if (vv->curtool->trigger != NULL) {
			AG_WidgetSetBool(vv->curtool->trigger, "state", 0);
		}
		if (vv->curtool->win != NULL) {
			AG_WindowHide(vv->curtool->win);
		}
		if (vv->curtool->pane != NULL) {
			AG_Widget *wt;
			AG_Window *pwin;

			OBJECT_FOREACH_CHILD(wt, vv->curtool->pane, ag_widget) {
				AG_ObjectDetach(wt);
				AG_ObjectDestroy(wt);
			}
			if ((pwin = AG_WidgetParentWindow(vv->curtool->pane))
			    != NULL) {
				AG_WindowUpdate(pwin);
			}
		}
		vv->curtool->vgv = NULL;
	}
	vv->curtool = ntool;

	if (ntool != NULL) {
		ntool->p = p;
		ntool->vgv = vv;

		if (ntool->trigger != NULL) {
			AG_WidgetSetBool(ntool->trigger, "state", 1);
		}
		if (ntool->win != NULL) {
			AG_WindowShow(ntool->win);
		}
#if 0
		if (ntool->pane != NULL && ntool->ops->edit != NULL) {
			AG_Window *pwin;

			ntool->ops->edit(ntool, ntool->pane);
			if ((pwin = AG_WidgetParentWindow(vv->curtool->pane))
			    != NULL) {
				AG_WindowUpdate(pwin);
			}
		}
#endif
		Snprintf(vv->status, sizeof(vv->status), "Tool: %s",
		    ntool->ops->name);
	} else {
		vv->status[0] = '\0';
	}
#if 0
	if ((pwin = AG_WidgetParentWindow(vv)) != NULL) {
		agView->winToFocus = pwin;
		AG_WidgetFocus(vv);
	}
#endif
}

VG_Tool *
VG_ViewFindTool(VG_View *vv, const char *name)
{
	VG_Tool *tool;

	TAILQ_FOREACH(tool, &vv->tools, tools) {
		if (strcmp(tool->ops->name, name) == 0)
			return (tool);
	}
	return (NULL);
}

VG_Tool *
VG_ViewFindToolByOps(VG_View *vv, const VG_ToolOps *ops)
{
	VG_Tool *tool;

	TAILQ_FOREACH(tool, &vv->tools, tools) {
		if (tool->ops == ops)
			return (tool);
	}
	return (NULL);
}

VG_Tool *
VG_ViewRegTool(VG_View *vv, const VG_ToolOps *ops, void *p)
{
	VG_Tool *t;

	t = Malloc(ops->len);
	t->ops = ops;
	t->vgv = vv;
	t->p = p;
	VG_ToolInit(t);
	TAILQ_INSERT_TAIL(&vv->tools, t, tools);
	return (t);
}

void
VG_ViewSetScale(VG_View *vv, float scale)
{
	vv->scale = MAX(scale,0.001f);
	vv->wPixel = 1.0/scale;
}

void
VG_ViewSetDefaultTool(VG_View *vv, VG_Tool *tool)
{
	vv->deftool = tool;
}

AG_WidgetClass vgViewClass = {
	{
		"AG_Widget:VG_View",
		sizeof(VG_View),
		{ 0,0 },
		Init,
		NULL,		/* free */
		NULL,		/* destroy */
		NULL,		/* load */
		NULL,		/* save */
		NULL		/* edit */
	},
	Draw,
	SizeRequest,
	SizeAllocate
};
