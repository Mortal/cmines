#ifndef SILENTSCREEN_H
#define SILENTSCREEN_H

#include <ncurses.h>
#include "Minefield.h"
#include "Screen.h"

class SilentScreen : public Screen<SilentScreen> {
	public:
		SilentScreen(Minefield *);
		void init();
		void deinit();
		void updatefield(const char *field);
		void updatetile(int idx);
		void vspeak(const char *fmt, va_list args);
		void mark(Mark mark);
		void resetmarks();
};

#endif
