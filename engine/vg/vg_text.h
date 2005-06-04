/*	$Csoft: vg_text.h,v 1.6 2004/05/28 22:31:58 vedge Exp $	*/
/*	Public domain	*/

#ifndef _AGAR_VG_TEXT_H_
#define _AGAR_VG_TEXT_H_
#include "begin_code.h"

#define VG_TEXT_MAX	 256
#define VG_TEXT_MAX_PTRS 32

struct vg_text_args {
	SDL_Surface *su;		/* Text surface */
	char text[VG_TEXT_MAX];		/* Text buffer */
	double angle;			/* Angle of rotation (deg) */
	enum vg_alignment align;	/* Alignment around vertex */
	void *ptrs[VG_TEXT_MAX_PTRS];	/* Pointers (for polling) */
	int  nptrs;
};

__BEGIN_DECLS
void	vg_text_align(struct vg *, enum vg_alignment);
void	vg_text_angle(struct vg *, double);
void	vg_printf(struct vg *, const char *, ...);
void	vg_pprintf(struct vg *, const char *, ...);
__END_DECLS

#include "close_code.h"
#endif /* _AGAR_VG_TEXT_H_ */
