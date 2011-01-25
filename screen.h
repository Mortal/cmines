#ifndef SCREEN_H
#define SCREEN_H

#include <ncurses.h>

typedef struct {
	WINDOW *field;
	WINDOW *speak;
} NC;

void delwins(Minefield *);
void setfieldsize(Minefield *);
void updatefield(Minefield *, const char *field);
void speak(Minefield *, const char *msg);

#endif
