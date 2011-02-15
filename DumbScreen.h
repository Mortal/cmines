#ifndef DUMBSCREEN_H
#define DUMBSCREEN_H

#include <ncurses.h>
#include "Minefield.h"
#include "Screen.h"

class DumbScreen : public Screen<DumbScreen> {
	public:
		DumbScreen(Minefield *);
		void init(Minefield *f);
		void deinit(Minefield *f);
		void updatefield(Minefield *f, const char *field);
		void updatetile(Minefield *f, int idx);
		void vspeak(Minefield *f, const char *fmt, va_list args);
		void mark(Minefield *f, int idx, int mark);
		void resetmarks(Minefield *f);
};

#endif
