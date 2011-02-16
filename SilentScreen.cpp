#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include <stdarg.h>
#include "Minefield.h"
#include "Screen.h"
#include "SilentScreen.h"

void SilentScreen::init() {
}

void SilentScreen::deinit() {
}

void SilentScreen::updatefield(const char *field) {
}

void SilentScreen::updatetile(int idx) {
}

void SilentScreen::vspeak(const char *fmt, va_list argp) {
	vprintf(fmt, argp);
}

void SilentScreen::mark(int idx, int mark) {
}

void SilentScreen::resetmarks() {
}

SilentScreen::SilentScreen(Minefield *f) {
}
