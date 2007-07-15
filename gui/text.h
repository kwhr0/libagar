/*	Public domain	*/

#ifndef _AGAR_WIDGET_TEXT_H_
#define _AGAR_WIDGET_TEXT_H_

#ifdef _AGAR_INTERNAL
#include <gui/button.h>
#include <gui/window.h>
#else
#include <agar/gui/button.h>
#include <agar/gui/window.h>
#endif

#include "begin_code.h"

#define AG_TEXT_STATES_MAX 32

enum ag_text_justify {
	AG_TEXT_LEFT,
	AG_TEXT_MIDDLE,
	AG_TEXT_RIGHT
};

enum ag_text_msg_title {
	AG_MSG_ERROR,
	AG_MSG_WARNING,
	AG_MSG_INFO
};

/* Cached glyph surface/texture information. */
typedef struct ag_glyph {
	char fontname[AG_OBJECT_NAME_MAX];
	int fontsize;			/* Font size in points */
	Uint32 color;			/* Glyph color */
	Uint32 ch;			/* Unicode character */
	Uint32 nrefs;			/* Reference count */
	SDL_Surface *su;		/* Rendered surface */
#ifdef HAVE_OPENGL
	Uint texture;			/* Rendered texture */
	float texcoord[4];
#endif
	SLIST_ENTRY(ag_glyph) glyphs;
} AG_Glyph;

/* Cached font */
typedef struct ag_font {
	struct ag_object obj;
	enum {
		AG_FONT_VECTOR,
		AG_FONT_BITMAP
	} type;
	int size;			/* Size in points */
	Uint flags;
#define AG_FONT_BOLD		0x01	/* Bold font */
#define AG_FONT_ITALIC		0x02	/* Italic font */
#define AG_FONT_UNDERLINE	0x04	/* Underlined */
#define AG_FONT_UPPERCASE	0x08	/* Force uppercase display */
	int height;			/* Body size in pixels */
	int ascent;			/* Ascent (relative to baseline) */
	int descent;			/* Descent (relative to baseline) */
	int lineskip;			/* Multiline y-increment */

	void *ttf;			/* TTF object */
	char bspec[32];			/* Bitmap font specification */
	SDL_Surface **bglyphs;		/* Bitmap glyphs */
	Uint nglyphs;			/* Bitmap glyph count */
	Uint32 c0, c1;			/* Bitmap glyph range */

	SLIST_ENTRY(ag_font) fonts;
} AG_Font;

typedef struct ag_text_state {
	AG_Font *font;			/* Font face */
	Uint32 color;			/* FG color (surfaceFmt) */
	Uint32 colorBG;			/* BG color (surfaceFmt) */
	enum ag_text_justify justify;	/* Justification mode */
} AG_TextState;

__BEGIN_DECLS
extern AG_Font *agDefaultFont;
extern int agTextFontHeight, agTextFontAscent, agTextFontDescent,
	   agTextFontLineSkip, agTextTabWidth, agTextBlinkRate;

int	 AG_TextInit(void);
void	 AG_TextParseFontSpec(const char *);
void	 AG_TextDestroy(void);

AG_Font	*AG_FetchFont(const char *, int, int);
void	 AG_FontDestroy(void *);

void	 AG_PushTextState(void);
void	 AG_PopTextState(void);

__inline__ void	AG_TextFont(AG_Font *);
__inline__ int  AG_TextFontLookup(const char *, int, Uint);
__inline__ void AG_TextJustify(enum ag_text_justify);
__inline__ void AG_TextColorVideo32(Uint32);
__inline__ void AG_TextColor32(Uint32);
__inline__ void AG_TextColorRGB(Uint8, Uint8, Uint8);
__inline__ void AG_TextColorRGBA(Uint8, Uint8, Uint8, Uint8);
__inline__ void AG_TextBGColorVideo32(Uint32);
__inline__ void AG_TextBGColor32(Uint32);
__inline__ void AG_TextBGColorRGB(Uint8, Uint8, Uint8);
__inline__ void AG_TextBGColorRGBA(Uint8, Uint8, Uint8, Uint8);
#define		AG_TextColor(name) AG_TextColorVideo32(AG_COLOR(name))
#define		AG_TextBGColor(name) AG_TextBGColorVideo32(AG_COLOR(name))

__inline__ SDL_Surface	*AG_TextFormat(const char *, ...);
__inline__ SDL_Surface	*AG_TextRender(const char *);
__inline__ SDL_Surface	*AG_TextRenderUCS4(const Uint32 *);
__inline__ void		 AG_TextSize(const char *, int *, int *);
__inline__ void		 AG_TextSizeUCS4(const Uint32 *, int *, int *);

void AG_TextMsg(enum ag_text_msg_title, const char *, ...)
	      FORMAT_ATTRIBUTE(printf, 2, 3)
	      NONNULL_ATTRIBUTE(2);
void AG_TextTmsg(enum ag_text_msg_title, Uint32, const char *, ...)
	       FORMAT_ATTRIBUTE(printf, 3, 4)
	       NONNULL_ATTRIBUTE(3);

#define AG_TextMsgFromError() \
	AG_TextMsg(AG_MSG_ERROR, "%s", AG_GetError())

void AG_TextEditFloat(double *, double, double, const char *,
		      const char *, ...)
		      FORMAT_ATTRIBUTE(printf, 5, 6)
		      NONNULL_ATTRIBUTE(5);
void AG_TextEditString(char **, size_t, const char *, ...)
		       FORMAT_ATTRIBUTE(printf, 3, 4)
		       NONNULL_ATTRIBUTE(3);

AG_Window *AG_TextPromptOptions(AG_Button **, Uint, const char *, ...)
		         	FORMAT_ATTRIBUTE(printf, 3, 4)
		          	NONNULL_ATTRIBUTE(3);
void AG_TextPromptString(const char *, void (*)(AG_Event *),
		         const char *, ...);
void AG_TextPromptDouble(const char *, const char *, double, double,
			 void (*)(AG_Event *), const char *, ...);

AG_Glyph *AG_TextRenderGlyph(Uint32);
void	  AG_TextUnusedGlyph(AG_Glyph *);
__END_DECLS

#include "close_code.h"
#endif	/* _AGAR_WIDGET_TEXT_H_ */
