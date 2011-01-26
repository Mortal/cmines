#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include "cmines.h"
#include "screen.h"

void delwins(Minefield *f) {
	if (!f->ncurses || f->testmode || f->ncursesdata == NULL) {
		return;
	}
	NC *nc = (NC *) f->ncursesdata;
	delwin(nc->field);
	delwin(nc->speak);
	free(f->ncursesdata);
	f->ncursesdata = NULL;
}

void setfieldsize(Minefield *f) {
	if (!f->ncurses || f->testmode) return;

	delwins(f);

	int width = f->outputwidth+1;
	int height = f->outputheight+1;

	/* get screen size */
	int ww, wh;
	getmaxyx(stdscr, wh, ww);

	/* allocate field of given size */
	WINDOW *field = newwin(height, width, 0, 0);

	WINDOW *speak;

	int cw = ww-width;
	if (cw < 0) cw = 0;
	int ch = wh-height;
	if (ch < 0) ch = 0;
	if (ch*10 > cw) {
		speak = newwin(0, 0, height, 0);
	} else {
		speak = newwin(0, 0, 0, width);
	}
	scrollok(speak, TRUE);
	wrefresh(field);
	wrefresh(speak);
	refresh();

	NC *nc = f->ncursesdata = (NC *) malloc(sizeof(NC));

	nc->field = field;
	nc->speak = speak;
}

void updatefield(Minefield *f, const char *field) {
	if (f->testmode) return;
	if (!f->ncurses) {
		printf("%s", field);
		return;
	}
	NC *nc = (NC *) f->ncursesdata;
	WINDOW *w = nc->field;
	mvwprintw(w, 0, 0, "%s", field);
	wrefresh(w);
	refresh();
}

void updatetile(Minefield *f, int idx) {
	if (f->testmode) return;
	if (!f->ncurses || f->ncursesdata == NULL) {
		return;
	}
	int row = outputrow(f, idxtocoords(f, idx));
	int column = outputcolumn(f, idxtocoords(f, idx));
	char c = tilechar(f->tiles+idx);
	NC *nc = (NC *) f->ncursesdata;
	WINDOW *w = nc->field;
	mvwaddch(w, row, column, c);
	wrefresh(w);
}

void speak(Minefield *f, const char *msg) {
	if (f->testmode) return;
	if (!f->ncurses) {
		printf("%s\n", msg);
		return;
	}
	NC *nc = (NC *) f->ncursesdata;
	WINDOW *s = nc->speak;
	wprintw(s, msg);
	wrefresh(s);
}
