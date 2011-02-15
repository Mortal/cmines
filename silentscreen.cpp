#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include <stdarg.h>
#include "Minefield.h"
#include "Screen.h"
#include "silentscreen.h"

static void silentinit(Minefield *f) {
}

static void silentdeinit(Minefield *f) {
}

static void silentupdatefield(Minefield *f, const char *field) {
}

static void silentupdatetile(Minefield *f, int idx) {
}

static void silentspeak(Minefield *f, const char *fmt, ...) {
	va_list argp;
	va_start(argp, fmt);
	vprintf(fmt, argp);
	va_end(argp);
}

static void silentmark(Minefield *f, int idx, int mark) {
}

static void silentresetmarks(Minefield *f) {
}

void silentscreen(Screen *s, Minefield *f) {
	s->init = &silentinit;
	s->deinit = &silentdeinit;
	s->updatefield = &silentupdatefield;
	s->updatetile = &silentupdatetile;
	s->speak = &silentspeak;
	s->mark = &silentmark;
	s->resetmarks = &silentresetmarks;
	s->data = NULL;
}
