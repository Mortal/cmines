#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include "cmines.h"
#include "Screen.h"
#include "dumbscreen.h"
#include "silentscreen.h"

static void dumbupdatefield(Minefield *f, const char *field) {
	printf("%s", field);
}

void dumbscreen(Screen *s, Minefield *f) {
	silentscreen(s, f);
	s->updatefield = &dumbupdatefield;
}
