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
		void init();
		void deinit();
		void updatefield(const char *field);
		void updatetile(int idx);
		void vspeak(const char *fmt, va_list args);
		void mark(int idx, int mark);
		void resetmarks();
		WINDOW *getField();
	private:
		NCscreen *nc;
		Minefield *f;
		void puttile(chtype ch, int mark);
		void updatetile_mark(int idx, int mark);
		void freemarks();
};

#endif
