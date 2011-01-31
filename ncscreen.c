#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include "cmines.h"
#include "Screen.h"
#include "ncscreen.h"

/* fields with no neighbours */
#define PAIR_VOID (1)
/* fields with >0 neighbours */
#define PAIR_WALL (2)
/* boundary between higher dimensions */
#define PAIR_BOUNDS (3)
/* unpressed fields */
#define PAIR_UNKNOWN (4)
/* flagged fields */
#define PAIR_FLAG (5)
/* pressed bombs */
#define PAIR_BOMB (6)

typedef struct {
	WINDOW *field;
	WINDOW *speak;
	bool colors;
} NC;

static void screendeinit(Minefield *f) {
	if (f->scr->data == NULL) return;
	NC *nc = (NC *) f->scr->data;
	endwin();
	delwin(nc->field);
	delwin(nc->speak);
	free(f->scr->data);
	f->scr->data = NULL;
}

static void screeninit(Minefield *f) {
	screendeinit(f);

	NC *nc = f->scr->data = (NC *) malloc(sizeof(NC));

	initscr();

	nc->colors = has_colors();

	if (nc->colors) {
		start_color();
		init_pair(PAIR_VOID, 0, 0);
		init_pair(PAIR_WALL, 0, 0);
		init_pair(PAIR_BOUNDS, 0, 0);
		init_pair(PAIR_UNKNOWN, 0, 0);
		init_pair(PAIR_FLAG, 0, 0);
		init_pair(PAIR_BOMB, 0, 0);
	}

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

static void puttile(Minefield *f, char tile) {
	NC *nc = (NC *) f->scr->data;
	if (nc == NULL) {
		putchar(tile);
		return;
	}
	WINDOW *w = nc->field;
	waddch(w, tile);
}

static void updatefield(Minefield *f, const char *field) {
	NC *nc = (NC *) f->scr->data;
	if (nc == NULL) {
		printf("%s", field);
		return;
	}
	WINDOW *w = nc->field;
	if (nc->colors) {
		wmove(w, 0, 0);
		while (*field != '\0') {
			puttile(f, *(field++));
		}
		return;
	}
	mvwprintw(w, 0, 0, "%s", field);
	wrefresh(w);
	refresh();
}

static void updatetile(Minefield *f, int idx) {
	NC *nc = (NC *) f->scr->data;
	if (nc == NULL) {
		return;
	}
	int row = outputrow(f, idxtocoords(f, idx));
	int column = outputcolumn(f, idxtocoords(f, idx));
	char c = tilechar(f->tiles+idx);
	WINDOW *w = nc->field;
	wmove(w, row, column);
	puttile(f, c);
	wrefresh(w);
}

static void speak(Minefield *f, const char *msg) {
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
