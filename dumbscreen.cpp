#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include "Minefield.h"
#include "Screen.h"
#include "dumbscreen.h"

void DumbScreen::updatefield(Minefield *f, const char *field) {
	printf("%s", field);
}

void DumbScreen::init(Minefield *f) {
}

void DumbScreen::deinit(Minefield *f) {
}

void DumbScreen::updatetile(Minefield *f, int idx) {
}

void DumbScreen::vspeak(Minefield *f, const char *fmt, va_list argp) {
	vprintf(fmt, argp);
}

void DumbScreen::mark(Minefield *f, int idx, int mark) {
}

void DumbScreen::resetmarks(Minefield *f) {
}

DumbScreen::DumbScreen(Minefield *f) {
}
