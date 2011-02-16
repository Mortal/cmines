#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include "Minefield.h"
#include "Screen.h"
#include "DumbScreen.h"

void DumbScreen::init() {
}

void DumbScreen::deinit() {
}

void DumbScreen::updatefield(const char *field) {
	printf("%s", field);
}

void DumbScreen::updatetile(int idx) {
}

void DumbScreen::vspeak(const char *fmt, va_list argp) {
	vprintf(fmt, argp);
}

void DumbScreen::mark(int idx, int mark) {
}

void DumbScreen::resetmarks() {
}

DumbScreen::DumbScreen(Minefield *f) {
}
