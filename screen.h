#ifndef SCREEN_H
#define SCREEN_H

#include <ncurses.h>
#include "cmines.h"

typedef void (*scrinit)(Minefield *f);
typedef void (*scrdeinit)(Minefield *);
typedef void (*scrupdatefield)(Minefield *, const char *field);
typedef void (*scrupdatetile)(Minefield *, int idx);
typedef void (*scrspeak)(Minefield *, const char *msg);

typedef struct _Screen {
	scrinit init;
	scrdeinit deinit;
	scrupdatefield updatefield;
	scrupdatetile updatetile;
	scrspeak speak;
} Screen;

void screen(Screen *, Minefield *);

#endif
