#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include <stdarg.h>
#include "Minefield.h"
#include "Screen.h"
#include "SilentScreen.h"

void SilentScreen::init(Minefield *f) {
}

void SilentScreen::deinit(Minefield *f) {
}

void SilentScreen::updatefield(Minefield *f, const char *field) {
}

void SilentScreen::updatetile(Minefield *f, int idx) {
}

void SilentScreen::vspeak(Minefield *f, const char *fmt, va_list argp) {
	vprintf(fmt, argp);
}

void SilentScreen::mark(Minefield *f, int idx, int mark) {
}

void SilentScreen::resetmarks(Minefield *f) {
}

SilentScreen::SilentScreen(Minefield *f) {
}
