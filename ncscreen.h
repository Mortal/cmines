#ifndef NCSCREEN_H
#define NCSCREEN_H

#include <ncurses.h>
#include "cmines.h"
#include "Screen.h"

struct NCscreen {
	WINDOW *field;
	WINDOW *speak;
	bool colors;
};

void ncscreen(Screen *, Minefield *);

#endif
