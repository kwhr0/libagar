/*	$Csoft: engine.h,v 1.28 2002/06/13 09:02:55 vedge Exp $	*/
/*	Public domain	*/

#ifndef _AGAR_ENGINE_H_
#define _AGAR_ENGINE_H_

#include <engine/mcconfig.h>

#include <pthread.h>
#include <stdlib.h>

#include <SDL.h>

#include <libfobj/fobj.h>

#include <engine/queue.h>
#include <engine/error.h>
#include <engine/debug.h>
#include <engine/object.h>
#include <engine/event.h>
#include <engine/anim.h>
#include <engine/world.h>
#include <engine/xcf.h>
#include <engine/view.h>

#if !defined(SERIALIZATION)
#define pthread_mutex_init(mu, attr)
#define pthread_mutex_destroy(mu)
#define pthread_mutex_lock(mu)
#define pthread_mutex_trylock(mu)
#define pthread_mutex_unlock(mu)
#endif

#define ENGINE_VERSION	"1.0-beta"

struct gameinfo {
	char	*prog;
	char	*name;
	char	*copyright;
	int	 ver[2];
};

#define ICON_GAME	0
#define ICON_MAPEDITION	1

#define ENGINE_START_GAME		0
#define ENGINE_START_MAP_EDITION	1

int		 engine_init(int, char **, struct gameinfo *, char *);
int		 engine_start(void);
void		 engine_stop(void);
void		 engine_destroy(void);

#endif	/* _AGAR_ENGINE_H_ */
