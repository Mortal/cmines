#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include "cmines.h"
#include "Screen.h"
#include "dumbscreen.h"

static void dumbinit(Minefield *f) {
}

static void dumbdeinit(Minefield *f) {
}

static void dumbupdatefield(Minefield *f, const char *field) {
	printf("%s", field);
}

static void dumbupdatetile(Minefield *f, int idx) {
}

static void dumbspeak(Minefield *f, const char *msg) {
	printf("%s\n", msg);
}

void dumbscreen(Screen *s, Minefield *f) {
	s->init = &dumbinit;
	s->deinit = &dumbdeinit;
	s->updatefield = &dumbupdatefield;
	s->updatetile = &dumbupdatetile;
	s->speak = &dumbspeak;
	s->data = NULL;
}
