#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include "cmines.h"
#include "Screen.h"
#include "ncscreen.h"

typedef struct {
	WINDOW *field;
	WINDOW *speak;
} NC;

void screendeinit(Minefield *f) {
	if (f->scr->data == NULL) return;
	NC *nc = (NC *) f->scr->data;
	endwin();
	delwin(nc->field);
	delwin(nc->speak);
	free(f->scr->data);
	f->scr->data = NULL;
}

void screeninit(Minefield *f) {
	screendeinit(f);

	NC *nc = f->scr->data = (NC *) malloc(sizeof(NC));

	initscr();

	int width = f->outputwidth+1;
	int height = f->outputheight+1;

	/* get screen size */
	int ww, wh;
	getmaxyx(stdscr, wh, ww);

	/* allocate field of given size */
	nc->field = newwin(height, width, 0, 0);

	int cw = ww-width;
	if (cw < 0) cw = 0;
	int ch = wh-height;
	if (ch < 0) ch = 0;
	if (ch*10 > cw) {
		nc->speak = newwin(0, 0, height, 0);
	} else {
		nc->speak = newwin(0, 0, 0, width);
	}
	scrollok(nc->speak, TRUE);
	wrefresh(nc->field);
	wrefresh(nc->speak);
	refresh();
}

void updatefield(Minefield *f, const char *field) {
	NC *nc = (NC *) f->scr->data;
	if (nc == NULL) {
		printf("%s", field);
		return;
	}
	WINDOW *w = nc->field;
	mvwprintw(w, 0, 0, "%s", field);
	wrefresh(w);
	refresh();
}

void updatetile(Minefield *f, int idx) {
	NC *nc = (NC *) f->scr->data;
	if (nc == NULL) {
		return;
	}
	int row = outputrow(f, idxtocoords(f, idx));
	int column = outputcolumn(f, idxtocoords(f, idx));
	char c = tilechar(f->tiles+idx);
	WINDOW *w = nc->field;
	mvwaddch(w, row, column, c);
	wrefresh(w);
}

void speak(Minefield *f, const char *msg) {
	NC *nc = (NC *) f->scr->data;
	if (nc == NULL) {
		printf("%s\n", msg);
		return;
	}
	WINDOW *s = nc->speak;
	wprintw(s, msg);
	wrefresh(s);
}

void ncscreen(Screen *s, Minefield *f) {
	s->init = &screeninit;
	s->deinit = &screendeinit;
	s->updatefield = &updatefield;
	s->updatetile = &updatetile;
	s->speak = &speak;
	s->data = NULL;
}
