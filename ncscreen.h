#ifndef NCSCREEN_H
#define NCSCREEN_H

#include <ncurses.h>
#include "cmines.h"
#include "Screen.h"

typedef struct _NCmark NCmark;

struct _NCmark {
	int idx;
	int mark;
	NCmark *next;
};

struct NCscreen {
	WINDOW *field;
	WINDOW *speak;
	bool colors;
	NCmark *mark;
};

void ncscreen(Screen *, Minefield *);

#endif
