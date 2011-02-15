#ifndef NCSCREEN_H
#define NCSCREEN_H

#include <ncurses.h>
#include "Minefield.h"
#include "Screen.h"

typedef struct _NCmark NCmark;

struct _NCmark {
	int idx;
	int mark;
	NCmark *next;
};

struct NCscreen {
	WINDOW *field;
	WINDOW *speak;
	bool colors;
	NCmark *mark;
};

class NCScreen : public Screen<NCScreen> {
	public:
		NCScreen(Minefield *);
		void init(Minefield *f);
		void deinit(Minefield *f);
		void updatefield(Minefield *f, const char *field);
		void updatetile(Minefield *f, int idx);
		void vspeak(Minefield *f, const char *fmt, va_list args);
		void mark(Minefield *f, int idx, int mark);
		void resetmarks(Minefield *f);
		WINDOW *getField();
	private:
		NCscreen *nc;
		void puttile(Minefield *f, chtype ch, int mark);
		void updatetile_mark(Minefield *f, int idx, int mark);
		void freemarks();
};

#endif
