#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include "Minefield.h"
#include "Screen.h"
#include "dumbscreen.h"

void DumbScreen::updatefield(Minefield *f, const char *field) {
	printf("%s", field);
}

DumbScreen::DumbScreen(Minefield *f) {
}
