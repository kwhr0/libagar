/*	Public domain	*/
/*
 * This application displays a set of standard Agar-GUI widgets.
 */

#include "agartest.h"

#include <agar/dev.h>

#include <stdio.h>
#include <math.h>
#include <string.h>

#include <agar/core/types.h>

#include <agar/config/ag_debug.h>
#include <agar/config/version.h>

typedef struct {
	AG_TestInstance _inherit;
	char textBuffer[128];
	char *someText;
	AG_TextElement *textElement;
} MyTestInstance;

/* Example callback for combo-selected. */
static void
ComboSelected(AG_Event *event)
{
	AG_TlistItem *ti = AG_TLIST_ITEM_PTR(1);

	AG_TextTmsg(AG_MSG_INFO, 500, "Selected Item: %s", ti->text);
}

/* Show the agar-dev "Preferences" dialog. */
static void
Preferences(AG_Event *event)
{
	DEV_ConfigShow();
}

static void
SetWordWrap(AG_Event *event)
{
	AG_Textbox *textbox = AG_TEXTBOX_PTR(1);
	int flag = AG_INT(2);

	AG_TextboxSetWordWrap(textbox, flag);
}

static void
TestMenuFn(AG_Event *event)
{
	const char *text = AG_STRING(1);

	AG_TextMsgS(AG_MSG_INFO, text);
}

static void
TableKeyDown(AG_Event *event)
{
	AG_Table *t = AG_TABLE_SELF();
	const int keysym = AG_INT(1);
	int m;

	switch (keysym) {
	case AG_KEY_DELETE:
		for (m = 0; m < t->m; m++) {
			if (AG_TableRowSelected(t,m))
				break;
		}
		if (m < t->m) {
			AG_Debug(t, "Deleting row # %d\n", m);
		}
		break;
	}
}

static void
SayHello(AG_Event *event)
{
	AG_Textbox *tb = AG_TEXTBOX_SELF();
	char *who = AG_TextboxDupString(tb);

	AG_TextInfo(NULL, "Hello, %s!", who);

	Free(who);
}

/*
 * Sample autocomplete for Textbox. Parse for "First Last" (or "First,Last")
 * and provide different suggestions for first names vs. last names. This
 * routine runs continuously (and independently of timers).
 */
static void
AutocompleteName(AG_Event *event)
{
	const char *dictFirst[] = {
		"Agnes", "Apu", "Artie", "Barbara", "Barry", "Bart", "Bernice",
		"Brandine", "Carl", "Cecil", "Cletus", "Count", "Disco",
		"Dondelinger", "Doris", "Drederick", "Eleanor", "Gerald", "Gil",
		"Gino", "Herman", "Homer", "Jasper", "Jebediah", "Jimbo",
		"Julius", "Kent", "Kirk", "Lenny", "Lisa", "Maggie", "Marge",
		"Martha", "Martin", "Manjula", "Marvin", "Maude", "Moe", "Murphy",
		"Ned", "Rainier", "Robert", "Rod", "Sarah", "Sideshow", "Todd",
		"\xC3\x9Cter", NULL
	}, **dp;
	const char *dictLast[] = {
		"Abernathy", "Beardly", "Bouvier", "Brockman", "Carlson",
		"Dracula", "Duffman", "Flanders", "Gunderson", "Harlan",
		"Hibbert", "Hermann", "Jones", "Krustofsky", "Moleman",
		"Monroe", "Nahasapeemapetilon", "Leonard", "Mel", "Prince",
		"Rollolinski", "Samson", "Simpson", "Skinner", "Springfield",
		"Spuckler", "Szyslak", "Stu", "Tatum", "Terwilliger",
		"Z\xC3\xB6rker", "Van Houten", "Wiggum", "Wolfcastle", "Ziff",
		NULL
	};
	AG_Editable *ed = AG_EDITABLE_SELF();
	AG_Tlist *tl = AG_TLIST_PTR(1);
	char *s = AG_EditableDupString(ed), *sp = s;
	const char *sFirst, *sLast;

	while (*sp == ' ' || *sp == '\t') {
		sp++;
	}
	sFirst = AG_Strsep(&sp, " ,");
	do {
		sLast = AG_Strsep(&sp, " ,");
	} while (sLast != NULL && *sLast == '\0');

	AG_TlistBegin(tl);

	if (sLast == NULL) {
		for (dp = dictFirst; *dp != NULL; dp++)
			if ((sFirst[0] == '\0' || sFirst[0] == '*') ||
			    AG_Strncasecmp(*dp, sFirst, strlen(sFirst)) == 0)
				AG_TlistAddPtr(tl, NULL, *dp, (void *)*dp);
	} else {
		for (dp = dictLast; *dp != NULL; dp++)
			if ((sLast[0] == '\0' || sLast[0] == '*') ||
			    AG_Strncasecmp(*dp, sLast, strlen(sLast)) == 0) {
				char *sd;

				asprintf(&sd, "%s %s", sFirst, *dp);
				AG_TlistAddPtr(tl, NULL, sd, (void *)sd);
				Free(sd);
			}
	}

	AG_TlistEnd(tl);

	Free(s);
}

/* Create more checkboxes under "Some checkboxes" */
static void
CreateMoreCheckboxes(AG_Event *_Nonnull event)
{
	AG_Button *bu = AG_BUTTON_SELF();
	AG_Box *box = AG_BOX_PTR(1);
	AG_Pane *pane = AG_PANE_PTR(2);

	AG_WidgetDisable(bu);

	AG_CheckboxNew(box, 0, "George Liquor");
	AG_CheckboxNew(box, 0, "Haggis McHaggis");
	AG_CheckboxNew(box, 0, "Kowalski, Bubba\n"
	                       "and Jiminy Lummox");
	AG_CheckboxNew(box, 0, "Wilbur Cobb");

	AG_PaneMoveDivider(pane, pane->dx + 50);
	AG_SetStyle(box, "font-size", "80%");
}

static int
TestGUI(void *obj, AG_Window *win)
{
	char path[AG_PATHNAME_MAX];
	MyTestInstance *ti = obj;
	AG_Box *hBox, *vBox;
	AG_Pane *hPane, *vPane;
	AG_Combo *com;
	AG_UCombo *ucom;
	AG_Textbox *tbox;
	AG_Surface *S;
	int i;

	/*
	 * Pane provides two Box containers which can be resized using
	 * a control placed in the middle.
	 */
	hPane = AG_PaneNewHoriz(win, AG_PANE_EXPAND);
	AG_PaneSetDivisionMin(hPane, 0, 50, 100);
	AG_PaneMoveDividerPct(hPane, 40);

	if (AG_ConfigFind(AG_CONFIG_PATH_DATA, "agar-1.bmp", path, sizeof(path)) == 0) {
		if ((S = AG_SurfaceFromBMP(path)) != NULL) {
			AG_PixmapFromSurface(hPane->div[0], 0, S);
		} else {
			S = AG_TextRender(AG_GetError());
			AG_PixmapFromSurface(hPane->div[0], 0, S);
			AG_SurfaceFree(S);
		}
	} else {
		S = AG_TextRender(AG_GetError());
		AG_PixmapFromSurface(hPane->div[0], 0, S);
		AG_SurfaceFree(S);
	}

	if (AG_ConfigFind(AG_CONFIG_PATH_DATA, "sq-agar.bmp", path, sizeof(path)) == 0) {
		AG_Surface *S = AG_SurfaceFromFile(path);
		int i, x,y;

		hBox = AG_BoxNewHoriz(hPane->div[0], AG_BOX_HOMOGENOUS |
		                                     AG_BOX_HFILL);
		for (i = 0; i < 5; i++) {
			for (y = 0; y < S->h; y++) {
				for (x = 0; x < S->w; x++) {
					AG_Pixel px = AG_SurfaceGet(S, x,y);
					AG_Color c ;
						
					AG_GetColor(&c, px, &S->format);
					c.a /= (1+i);
					AG_SurfacePut(S, x,y,
					    AG_MapPixel(&S->format, &c));
				}
			}
			AG_PixmapFromSurface(hBox, 0, S);
		}
	} else {
		S = AG_TextRender(AG_GetError());
		AG_PixmapFromSurface(hPane->div[0], 0, S);
		AG_SurfaceFree(S);
	}
	
	/*
	 * AG_Label(3) displays either a static text label, or a dynamically
	 * updated one. Polled (dynamic) labels use a special format documented
	 * in AG_String(3).
	 */
	{
		AG_AgarVersion av;
		AG_Label *lbl;

		AG_GetVersion(&av);

		/* A static label */
		lbl = AG_LabelNew(hPane->div[0], 0,
		    "Agar v%d.%d.%d ( " AGSI_FRAK "%s" AGSI_RST " )",
		    av.major, av.minor, av.patch,
		    av.release ? av.release : "dev");

		/* A dynamically-updated label. */
		lbl = AG_LabelNewPolled(hPane->div[0], AG_LABEL_HFILL,
		    "Window is at %i,%i (%ux%u)\n",
		    &AGWIDGET(win)->x,
		    &AGWIDGET(win)->y,
		    &AGWIDGET(win)->w,
		    &AGWIDGET(win)->h);

		AG_LabelSizeHint(lbl, 1,
		    "This is a polled label\n"
		    "Window is at 000,000 (000x000)");

		AG_LabelJustify(lbl, AG_TEXT_CENTER);
	}

	/*
	 * Pane provides two Box containers which can be resized using
	 * a control placed in the middle.
	 */
	vPane = AG_PaneNewVert(hPane->div[0], AG_PANE_EXPAND);

	/*
	 * Box is a general-purpose widget container. AG_BoxNewHoriz() creates
	 * a container which packs its widgets horizontally.
	 */
	hBox = AG_BoxNewHoriz(vPane->div[0], AG_BOX_HOMOGENOUS | AG_BOX_HFILL);
	{
		/*
		 * The Button widget is a simple push-button. It is typically
		 * used to trigger events, but it can also bind its state to
		 * an boolean (integer) value or a bitmask.
		 */
		for (i = 0; i < 5; i++)
			AG_ButtonNewS(hBox, 0, AG_Printf("%c", 0x41+i));
	}

	hBox = AG_BoxNewHoriz(vPane->div[0], AG_BOX_HFILL);
	AG_BoxSetHorizAlign(hBox, AG_BOX_CENTER);
	AG_BoxSetVertAlign(hBox, AG_BOX_CENTER);
	{
		/* Radio button group */
		vBox = AG_BoxNewVert(hBox, 0);
		AG_BoxSetLabel(vBox, "Radio group:");
		{
			const char *radioItems[] = {
				"Homer\n"
				"(" AGSI_ITALIC "Simpson" AGSI_RST ")",
				"Marge",
				"Bart",
				"Lisa",
				"Maggie",
				NULL
			};
			AG_RadioNew(vBox, 0, radioItems);
		}
	
		vBox = AG_BoxNewVert(hBox, 0);
		AG_BoxSetLabel(vBox, "Checkboxes:");
		{
			AG_Button *btn;

			/*
			 * The Checkbox widget can bind to boolean values
			 * and bitmasks.
			 */
			AG_CheckboxNew(vBox, 0, "Ren H" "\xC3\xB6" "ek");
			AG_CheckboxNew(vBox, 0, "Stimpson "
			                        AGSI_ITALIC "J." AGSI_RST
						" Cat");
			AG_CheckboxNew(vBox, 0, "Mr. " AGSI_BOLD "Horse" AGSI_RST);
			AG_CheckboxNew(vBox, 0, AGSI_YEL AGSI_BOLD "P" AGSI_RST
			                        "owdered "
						AGSI_YEL AGSI_BOLD "T" AGSI_RST
						"oast "
						AGSI_YEL AGSI_BOLD "M" AGSI_RST
						"an");

			btn = AG_ButtonNewFn(vBox, 0, "Create More ...",
			    CreateMoreCheckboxes, "%p,%p", vBox, vPane);

			AG_SetStyle(btn, "padding", "2");
		}
	}

	/*
	 * The Combo widget is a textbox widget with a expander button next
	 * to it. The button triggers a popup window which displays a list
	 * (using the AG_Tlist(3) widget).
	 */
	com = AG_ComboNew(vPane->div[1], AG_COMBO_HFILL, "Combo: ");
	AG_ComboSizeHint(com, "Item #00 ", 10);
	AG_SetEvent(com, "combo-selected", ComboSelected, NULL);

	/* UCombo is a variant of Combo which looks like a single button. */
	ucom = AG_UComboNew(vPane->div[1], AG_UCOMBO_HFILL);
	AG_UComboSizeHint(ucom, "Item #1234", 5);

	/* Populate the Tlist displayed by the combo widgets we just created. */
	for (i = 0; i < 50; i++) {
		AG_Color c;
		char text[32];
		AG_TlistItem *it;

		/* This is more efficient than AG_Printf() */
		AG_Strlcpy(text, "Item #", sizeof(text));
		AG_StrlcatInt(text, i, sizeof(text));

		it = AG_TlistAddS(com->list, NULL, text);
		if ((i % 25) == 0) {
			it->fontFlags |= AG_FONT_UNDERLINE;
		} else if ((i % 10) == 0) {
			it->fontFlags |= AG_FONT_BOLD;
		} else if ((i % 5) == 0) {
			it->fontFlags |= AG_FONT_ITALIC;
		}

		AG_ColorRGB_8(&c, 240, 240, 255 - (i << 2));
		AG_TlistSetColor(com->list, it, &c);

		AG_TlistAddS(ucom->list, NULL, text);
	}

	/*
	 * Numerical binds to an integral or floating-point number.
	 * It can also provides built-in unit conversion (see AG_Units(3)).
	 */
	{
		AG_Numerical *num;
		static float myFloat = 1.0f;
		static int myInt = 50;

		num = AG_NumericalNewS(vPane->div[1],
		    AG_NUMERICAL_EXCL | AG_NUMERICAL_HFILL,
		    "cm", "Numerical: ");
		AG_BindFloat(num, "value", &myFloat);
		AG_SetFloat(num, "inc", 1.0f);

		num = AG_NumericalNewS(vPane->div[1],
		    AG_NUMERICAL_EXCL | AG_NUMERICAL_HFILL,
		    NULL, "Integer: ");
		AG_BindInt(num, "value", &myInt);
		AG_SetInt(num, "min", -100);
		AG_SetInt(num, "max", +100);
	}

	/*
	 * Textbox is a single or multiline text edition widget. It can bind
	 * to fixed-size or dynamically-sized buffers in any character set
	 * encoding.
	 */
	{
		ti->textBuffer[0] = '\0';
		ti->textElement = AG_TextNew(100);

		/* Create a textbox bound to a fixed-size buffer */
		tbox = AG_TextboxNew(vPane->div[0],
		    AG_TEXTBOX_EXCL | AG_TEXTBOX_HFILL,
		    "Textbox: ");
		AG_TextboxSetPlaceholderS(tbox, "First & Last Name");
		AG_TextboxAutocomplete(tbox, AutocompleteName, NULL);
		AG_TextboxBindUTF8(tbox, ti->textBuffer, sizeof(ti->textBuffer));
		AG_SetEvent(tbox, "textbox-return", SayHello, "%p", tbox);

#ifdef AG_UNICODE
		/*
		 * Create a textbox connected to a multilingual, dynamically
		 * sized text element.
		 */
		tbox = AG_TextboxNew(vPane->div[1],
		    AG_TEXTBOX_EXCL | AG_TEXTBOX_MULTILINGUAL | AG_TEXTBOX_HFILL,
		    "TextElement: ");

		AG_TextboxSetString(tbox, "Hello");

		AG_TextSetEntS(ti->textElement, AG_LANG_EN, "Hello");
		AG_TextSetEntS(ti->textElement, AG_LANG_FR, "Bonjour");
		AG_TextSetEntS(ti->textElement, AG_LANG_DE, "Guten tag");

		AG_TextboxBindText(tbox, ti->textElement);
#endif
	}

	/* Create a horizontal separator */
	AG_SeparatorNewHoriz(vPane->div[1]);

	/*
	 * Scrollbar provides "value", "min" and "max" bindings which can be
	 * connected to integers or floating-point variables. Progressbar and
	 * Slider offer similar interfaces.
	 */
	{
		static int myVal = 50, myMin = -100, myMax = 100, myVisible = 0;
		AG_Scrollbar *sb;
		AG_Slider *sl;
		AG_ProgressBar *pb;

		AG_LabelNewS(vPane->div[1], 0, "Scrollbar:");
		sb = AG_ScrollbarNewHoriz(vPane->div[1], AG_SCROLLBAR_EXCL |
		                                         AG_SCROLLBAR_HFILL);
		AG_BindInt(sb, "value", &myVal);
		AG_BindInt(sb, "min", &myMin);
		AG_BindInt(sb, "max", &myMax);
		AG_BindInt(sb, "visible", &myVisible);
		AG_SetInt(sb, "inc", 10);

		AG_LabelNewS(vPane->div[1], 0, "Slider:");
		sl = AG_SliderNew(vPane->div[1], AG_SLIDER_HORIZ, AG_SLIDER_EXCL |
		                                                  AG_SLIDER_HFILL);
		AG_BindInt(sl, "value", &myVal);
		AG_BindInt(sl, "min", &myMin);
		AG_BindInt(sl, "max", &myMax);
		AG_SetInt(sl, "inc", 10);

		AG_LabelNewS(vPane->div[1], 0, "ProgressBar:");
		pb = AG_ProgressBarNewHoriz(vPane->div[1], AG_PROGRESS_BAR_EXCL |
		                                           AG_PROGRESS_BAR_SHOW_PCT |
		                                           AG_PROGRESS_BAR_HFILL);
		AG_BindInt(pb, "value", &myVal);
		AG_BindInt(pb, "min", &myMin);
		AG_BindInt(pb, "max", &myMax);
	}

	/*
	 * Notebook provides multiple containers which can be selected by
	 * the user.
	 */
	{
		AG_Notebook *nb;
		AG_NotebookTab *nt;
		AG_Table *table;
		AG_Menu *menu;
		AG_MenuItem *m, *mSub;
		static int myInt[2];
	
		/* Create a test menu */
		menu = AG_MenuNew(hPane->div[1], AG_MENU_HFILL);
		m = AG_MenuNode(menu->root, "File", NULL);
		{
			AG_MenuAction(m, "Agar Preferences...", agIconGear.s,
			    Preferences, NULL);
		}
		m = AG_MenuNode(menu->root, "Test Menu", NULL);
		{
			/* A disabled menu item */
			AG_MenuState(m, 0);
			AG_MenuNode(m, "Disabled Submenu A", agIconMagnifier.s);
			AG_MenuState(m, 1);
			AG_MenuSeparator(m);

			/* Menu item with child entries */
			mSub = AG_MenuNode(m, "Submenu B", agIconGear.s);
			AG_MenuAction(mSub, "Submenu C", agIconLoad.s,
			    TestMenuFn, "%s", "Submenu C selected!");
			AG_MenuAction(mSub, "Submenu D", agIconSave.s,
			    TestMenuFn, "%s", "Submenu D selected!");
			AG_MenuAction(mSub, "Submenu E", agIconTrash.s,
			    TestMenuFn, "%s", "Submenu E selected!");
			AG_MenuIntBool(mSub, "Togglable Binding #1", agIconUp.s, &myInt[0], 0);

			AG_MenuSeparator(m);
			AG_MenuSectionS(m, "Non-selectable text");
			AG_MenuSeparator(m);

			AG_MenuIntBool(m, "Togglable Binding #1", agIconUp.s, &myInt[0], 0);
			AG_MenuIntBool(m, "Togglable Binding #2", agIconDown.s, &myInt[1], 0);
			AG_MenuIntBool(m, "Inverted Binding #2", agIconDown.s, &myInt[1], 1);
		}

		nb = AG_NotebookNew(hPane->div[1], AG_NOTEBOOK_EXPAND);

		nt = AG_NotebookAdd(nb, "Some table", AG_BOX_VERT);
		{
			float f;

			/*
			 * AG_Table displays a set of cells organized in
			 * rows and columns. It is optimized for cases where
			 * the table is static or needs to be repopulated
			 * periodically.
			 */
			table = AG_TableNew(nt, AG_TABLE_EXPAND | AG_TABLE_MULTI);

			AG_TableAddCol(table, "x", "33%", NULL);
			AG_TableAddCol(table, "sin(x)", "33%", NULL);
			AG_TableAddCol(table, "cos(x)", "33%", NULL);

			AG_SetStyle(table, "font-size", "80%");

			for (f = 0.0f; f < 60.0f; f += 0.3f) {
				/*
				 * Insert a Table row for sin(f) and cos(f).
				 * The directives of the format string are
				 * documented in AG_Table(3).
				 */
				AG_TableAddRow(table, "%.02f:%.02f:%.02f",
				    f, sin(f), cos(f));
			}
			{
				const char *selModes[] = {
				    "Select\nby\nRow",
				    "Select\nby\nCell",
				    "Select\nby\nColumn",
				    NULL
				};
				AG_Radio *rad;

				rad = AG_RadioNewUint(nt, 0, selModes, &table->selMode);
				AG_RadioSetDisposition(rad, AG_RADIO_HORIZ);
				AG_SetStyle(rad, "font-size", "80%");
			}
			

			vBox = AG_BoxNewVert(nt, 0);
			AG_SetStyle(vBox, "font-size", "80%");
			{
				AG_CheckboxNewFlag(vBox, 0, "Select multiple\n(with ctrl/shift)",
				    &table->flags, AG_TABLE_MULTI);

				AG_CheckboxNewFlag(vBox, 0, "Select multiple always",
				    &table->flags, AG_TABLE_MULTITOGGLE);

				AG_CheckboxNewFlag(vBox, 0, "Highlight columns",
				    &table->flags, AG_TABLE_HIGHLIGHT_COLS);
			}

			/*
			 * Append an event handler for the DELETE function.
			 */
			AG_AddEvent(table, "key-down", TableKeyDown, NULL);
		}
		
		nt = AG_NotebookAdd(nb, "Some text", AG_BOX_VERT);
		{
			char path[AG_PATHNAME_MAX];
			AG_Size size, bufSize;
			FILE *f;

			/*
			 * Textboxes with the MULTILINE flag provide basic
			 * text edition functionality. The CATCH_TAB flag
			 * causes the widget to receive TAB key events
			 * (normally used to focus other widgets).
			 */
			tbox = AG_TextboxNewS(nt,
			    AG_TEXTBOX_MULTILINE | AG_TEXTBOX_CATCH_TAB |
			    AG_TEXTBOX_EXPAND | AG_TEXTBOX_EXCL, NULL);
			AG_WidgetSetFocusable(tbox, 1);

			/*
			 * Load the contents of this file into a buffer. Make
			 * the buffer a bit larger so the user can try
			 * entering text.
			 */
			if (AG_ConfigFind(AG_CONFIG_PATH_DATA, "loss.txt",
			    path, sizeof(path)) == 0) {
				if ((f = fopen(path, "r")) != NULL) {
					fseek(f, 0, SEEK_END);
					size = ftell(f);
					fseek(f, 0, SEEK_SET);
					bufSize = size+1024;
					ti->someText = AG_Malloc(bufSize);
					(void)fread(ti->someText, size, 1, f);
					fclose(f);
					ti->someText[size] = '\0';
				} else {
					ti->someText = Strdup(path);
					bufSize = strlen(ti->someText)+1;
				}
			} else {
				ti->someText = AG_Strdup("loss.txt not found");
				bufSize = strlen(ti->someText)+1;
			}
	
			/*
			 * Bind the buffer's contents to the Textbox. The
			 * size argument to AG_TextboxBindUTF8() must include
			 * space for the terminating NUL.
			 */
			AG_TextboxBindUTF8(tbox, ti->someText, bufSize);
	
			/* Add a word wrapping control */
			AG_CheckboxNewFn(nt, 0, "Word wrapping",
			    SetWordWrap, "%p", tbox);
		}
		
		nt = AG_NotebookAdd(nb, "Empty tab", AG_BOX_VERT);
	}
	return (0);
}

static int
Init(void *obj)
{
	MyTestInstance *ti = obj;

	ti->textBuffer[0] = '\0';
	ti->someText = NULL;
	DEV_InitSubsystem(0);
	return (0);
}

static void
Destroy(void *obj)
{
	MyTestInstance *ti = obj;

	Free(ti->someText);
	AG_TextFree(ti->textElement);
}

const AG_TestCase widgetsTest = {
	"widgets",
	N_("Display various Agar-GUI widgets"),
	"1.6.0",
	0,
	sizeof(MyTestInstance),
	Init,
	Destroy,
	NULL,		/* test */
	TestGUI,
	NULL		/* bench */
};
