/*	Public domain	*/
/*
 * Generic graphics/input driver framework.
 */

#ifndef _AGAR_GUI_DRV_H_
#define _AGAR_GUI_DRV_H_
#include <agar/gui/begin.h>

enum ag_driver_type {
	AG_FRAMEBUFFER,			/* Direct rendering to frame buffer */
	AG_VECTOR			/* Vector drawing */
};
enum ag_driver_wm_type {
	AG_WM_SINGLE,			/* Single display */
	AG_WM_MULTIPLE			/* Multiple windows */
};

struct ag_widget;
struct ag_window;
struct ag_glyph;
struct ag_cursor;

/* Generic graphics driver class */
typedef struct ag_driver_class {
	struct ag_object_class _inherit;
	const char *name;			/* Short name */
	enum ag_driver_type type;		/* Driver type */
	enum ag_driver_wm_type wm;		/* Window manager type */
	Uint flags;
#define AG_DRIVER_OPENGL	0x01		/* Supports OpenGL calls */
#define AG_DRIVER_SDL		0x02		/* Supports SDL calls */
#define AG_DRIVER_TEXTURES	0x04		/* Support texture ops */

	/* Initialization */
	int  (*open)(void *drv, const char *spec);
	void (*close)(void *drv);
	int  (*getDisplaySize)(Uint *w, Uint *h);
	/* Event processing */
	void (*beginEventProcessing)(void *drv);
	int  (*processEvents)(void *drv);
	void (*genericEventLoop)(void *drv);
	void (*endEventProcessing)(void *drv);
	/* GUI rendering */
	void (*beginRendering)(void *drv);
	void (*renderWindow)(struct ag_window *);
	void (*endRendering)(void *drv);
	/* Primitives */
	void (*fillRect)(void *drv, AG_Rect r, AG_Color c);
	/* Update video region (rendering context; FB driver specific) */
	void (*updateRegion)(void *drv, AG_Rect r);
	/* Texture operations (GL driver specific) */
	int  (*uploadTexture)(Uint *, AG_Surface *, AG_TexCoord *);
	int  (*updateTexture)(Uint, AG_Surface *);
	void (*deleteTexture)(void *drv, Uint);
	/* Request a specific refresh rate (driver specific) */
	int (*setRefreshRate)(void *drv, int fps);
	/* Clipping and blending control (rendering context) */
	void (*pushClipRect)(void *drv, AG_Rect r);
	void (*popClipRect)(void *drv);
	void (*pushBlendingMode)(void *drv, AG_BlendFn srcFn, AG_BlendFn dstFn);
	void (*popBlendingMode)(void *drv);
	/* Hardware cursor operations */
	int  (*createCursor)(void *drv, struct ag_cursor *curs);
	void (*freeCursor)(void *drv, struct ag_cursor *curs);
	int  (*pushCursor)(void *drv, struct ag_cursor *curs);
	void (*popCursor)(void *drv);
	int  (*getCursorVisibility)(void *drv);
	void (*setCursorVisibility)(void *drv, int flag);
	/* Widget surface operations (rendering context) */
	void (*blitSurface)(void *drv, struct ag_widget *wid, AG_Surface *s, int x, int y);
	void (*blitSurfaceFrom)(void *drv, struct ag_widget *wid, struct ag_widget *widSrc, int s, AG_Rect *r, int x, int y);
	void (*blitSurfaceGL)(void *drv, struct ag_widget *wid, AG_Surface *s, float w, float h);
	void (*blitSurfaceFromGL)(void *drv, struct ag_widget *wid, int s, float w, float h);
	void (*blitSurfaceFlippedGL)(void *drv, struct ag_widget *wid, int s, float w, float h);
	void (*backupSurfaces)(void *drv, struct ag_widget *wid);
	void (*restoreSurfaces)(void *drv, struct ag_widget *wid);
	int  (*renderToSurface)(void *drv, struct ag_widget *wid, AG_Surface **su);
	/* Rendering operations (rendering context) */
	void (*putPixel)(void *drv, int x, int y, AG_Color c);
	void (*putPixel32)(void *drv, int x, int y, Uint32 c);
	void (*putPixelRGB)(void *drv, int x, int y, Uint8 r, Uint8 g, Uint8 b);
	void (*blendPixel)(void *drv, int x, int y, AG_Color c, AG_BlendFn fnSrc, AG_BlendFn fnDst);
	void (*drawLine)(void *drv, int x1, int y1, int x2, int y2, AG_Color C);
	void (*drawLineH)(void *drv, int x1, int x2, int y, AG_Color C);
	void (*drawLineV)(void *drv, int x, int y1, int y2, AG_Color C);
	void (*drawLineBlended)(void *drv, int x1, int y1, int x2, int y2, AG_Color C, AG_BlendFn fnSrc, AG_BlendFn fnDst);
	void (*drawArrowUp)(void *drv, int x0, int y0, int h, AG_Color C[2]);
	void (*drawArrowDown)(void *drv, int x0, int y0, int h, AG_Color C[2]);
	void (*drawArrowLeft)(void *drv, int x0, int y0, int h, AG_Color C[2]);
	void (*drawArrowRight)(void *drv, int x0, int y0, int h, AG_Color C[2]);
	void (*drawBoxRounded)(void *drv, AG_Rect r, int z, int rad, AG_Color C[3]);
	void (*drawBoxRoundedTop)(void *drv, AG_Rect r, int z, int rad, AG_Color C[3]);
	void (*drawCircle)(void *drv, int x, int y, int r, AG_Color C);
	void (*drawCircle2)(void *drv, int x, int y, int r, AG_Color C);
	void (*drawRectFilled)(void *drv, AG_Rect r, AG_Color C);
	void (*drawRectBlended)(void *drv, AG_Rect r, AG_Color C, AG_BlendFn fnSrc, AG_BlendFn fnDst);
	void (*drawRectDithered)(void *drv, AG_Rect r, AG_Color C);
	void (*drawFrame)(void *drv, AG_Rect r, AG_Color C[2]);
	void (*drawGlyph)(void *drv, struct ag_glyph *, int x, int y);
} AG_DriverClass;

/* Generic driver instance. */
typedef struct ag_driver {
	struct ag_object _inherit;
	Uint id;			/* Numerical instance ID */
	Uint flags;
	AG_Surface *sRef;		/* "Reference" surface */
	AG_PixelFormat *videoFmt;	/* Video pixel format (for
					   packed-pixel FB modes) */
	struct ag_keyboard *kbd;	/* Main keyboard device */
	struct ag_mouse *mouse;		/* Main mouse device */

	struct ag_cursor *activeCursor;	/* Effective cursor */
	struct ag_cursor *cursors;	/* Registered mouse cursors */
	Uint             nCursors;
} AG_Driver;

#define AGDRIVER(obj)		((AG_Driver *)(obj))
#define AGDRIVER_CLASS(obj)	((struct ag_driver_class *)(AGOBJECT(obj)->cls))
#define AGDRIVER_SINGLE(drv)	(AGDRIVER_CLASS(drv)->wm == AG_WM_SINGLE)
#define AGDRIVER_MULTIPLE(drv)	(AGDRIVER_CLASS(drv)->wm == AG_WM_MULTIPLE)
#define AGDRIVER_BOUNDED_WIDTH(win,x) (((x) < 0) ? 0 : \
                                      ((x) > AGWIDGET(win)->w) ? (AGWIDGET(win)->w - 1) : (x))
#define AGDRIVER_BOUNDED_HEIGHT(win,y) (((y) < 0) ? 0 : \
                                       ((y) > AGWIDGET(win)->h) ? (AGWIDGET(win)->h - 1) : (y))

__BEGIN_DECLS
extern AG_ObjectClass agDriverClass;

extern AG_Object      agDrivers;	/* Drivers VFS */
extern AG_DriverClass *agDriverOps;	/* Driver class */
extern AG_Driver      *agDriver;  	/* Driver instance */
extern void *agDriverList[];		/* Available drivers (AG_DriverClass) */
extern Uint  agDriverListSize;
extern int   agRenderingContext;	/* Running in rendering context? */

AG_Driver *AG_DriverOpen(AG_DriverClass *);
void       AG_DriverClose(AG_Driver *);
void       AG_ViewCapture(void);

/* Lookup a driver instance by ID */
static __inline__ AG_Driver *
AG_GetDriverByID(Uint id)
{
	AG_Driver *drv;

	AGOBJECT_FOREACH_CHILD(drv, &agDrivers, ag_driver) {
		if (drv->id == id)
			return (drv);
	}
	return (NULL);
}

/* Update a video region (FB drivers only). */
static __inline__ void
AG_ViewUpdateFB(const AG_Rect2 *r2)
{
	AG_Rect r;
	r.x = r2->x1;
	r.y = r2->y1;
	r.w = r2->w;
	r.h = r2->h;
	agDriverOps->updateRegion(agDriver, r);
}

/* Enter GUI rendering context. */
static __inline__ void
AG_BeginRendering(AG_Driver *drv)
{
#ifdef AG_DEBUG
	agRenderingContext = 1;
#endif
	AGDRIVER_CLASS(drv)->beginRendering(drv);
}

/* Leave GUI rendering context. */
static __inline__ void
AG_EndRendering(AG_Driver *drv)
{
	AGDRIVER_CLASS(drv)->endRendering(drv);
#ifdef AG_DEBUG
	agRenderingContext = 0;
#endif
}

/* Create a texture from a surface (GL drivers). */
static __inline__ Uint
AG_SurfaceTexture(AG_Surface *su, AG_TexCoord *tc)
{
	Uint texid;

	if (agDriverOps->uploadTexture(&texid, su, tc) == -1) {
		AG_FatalError(NULL);
	}
	return (texid);
}

/* Update texture contents from a surface (GL drivers). */
static __inline__ void
AG_UpdateTexture(AG_Surface *su, int texid)
{
	if (agDriverOps->updateTexture((Uint)texid, su) == -1)
		AG_FatalError(NULL);
}

#ifdef AG_LEGACY
extern AG_Driver *agView;  	/* Pre-1.4 */
#endif
__END_DECLS

#include <agar/gui/drv_mw.h>
#include <agar/gui/drv_sw.h>

__BEGIN_DECLS
/* Query a driver for available display area in pixels. */
static __inline__ int
AG_GetDisplaySize(void *drv, Uint *w, Uint *h)
{
	switch (AGDRIVER_CLASS(drv)->wm) {
	case AG_WM_SINGLE:
		*w = AGDRIVER_SW(drv)->w;
		*h = AGDRIVER_SW(drv)->h;
		return (0);
	case AG_WM_MULTIPLE:
		return AGDRIVER_CLASS(drv)->getDisplaySize(w, h);
	}
	return (-1);
}
__END_DECLS

#include <agar/gui/close.h>
#endif /* _AGAR_GUI_DRV_H_ */
