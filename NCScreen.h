#ifndef NCSCREEN_H
#define NCSCREEN_H

#include <ncurses.h>
#include <queue>
#include "Minefield.h"
#include "Screen.h"

class NCScreen : public Screen<NCScreen> {
	public:
		NCScreen(Minefield *);
		void init();
		void deinit();
		void updatefield(const char *field);
		void updatetile(int idx);
		void vspeak(const char *fmt, va_list args);
		void mark(Mark);
		void resetmarks();
		WINDOW *getField();
	private:
		Minefield *f;

		WINDOW *field;
		WINDOW *speak;
		bool colors;
		std::queue<Mark> marks;

		void puttile(chtype ch, int mark);
		void updatetile_mark(int idx, int mark);
		void freemarks();
};

#endif
