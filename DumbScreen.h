#ifndef DUMBSCREEN_H
#define DUMBSCREEN_H

#include <ncurses.h>
#include "Minefield.h"
#include "Screen.h"

class DumbScreen : public Screen<DumbScreen> {
	public:
		DumbScreen(Minefield *);
		void init();
		void deinit();
		void updatefield(const char *field);
		void updatetile(int idx);
		void vspeak(const char *fmt, va_list args);
		void mark(Mark);
		void resetmarks();
};

#endif
