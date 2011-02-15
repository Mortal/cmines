#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include <stdarg.h>
#include "cmines.h"
#include "Screen.h"
#include "ncscreen.h"
#include "silentscreen.h"

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
/* primary mark */
#define PAIR_PRIMARY_MARK (7)
/* secondary mark */
#define PAIR_SECONDARY_MARK (8)

typedef struct NCscreen NC;

static void freemarks(NC *nc) {
	NCmark *mark = nc->mark;
	while (mark != NULL) {
		NCmark *m = mark;
		mark = m->next;
		delete m;
	}
	nc->mark = NULL;
}

static void screendeinit(Minefield *f) {
	if (f->scr->data == NULL) return;
	NC *nc = (NC *) f->scr->data;
	curs_set(1);
	endwin();
	delwin(nc->field);
	delwin(nc->speak);

	freemarks(nc);

	delete f->scr->data;
	f->scr->data = NULL;
}

static void screeninit(Minefield *f) {
	screendeinit(f);

	NC *nc;
	f->scr->data = nc = new NC;

	initscr();
	cbreak();
	noecho();
	curs_set(0);

	nc->colors = has_colors();

	if (nc->colors) {
		start_color();
		init_pair(PAIR_VOID, COLOR_WHITE, COLOR_BLACK);
		init_pair(PAIR_WALL, COLOR_YELLOW, COLOR_BLACK);
		init_pair(PAIR_BOUNDS, COLOR_BLUE, COLOR_BLACK);
		init_pair(PAIR_UNKNOWN, COLOR_CYAN, COLOR_BLACK);
		init_pair(PAIR_FLAG, COLOR_RED, COLOR_BLACK);
		init_pair(PAIR_BOMB, COLOR_BLACK, COLOR_RED);
		init_pair(PAIR_PRIMARY_MARK, COLOR_RED, COLOR_WHITE);
		init_pair(PAIR_SECONDARY_MARK, COLOR_BLACK, COLOR_BLUE);
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

	nc->mark = NULL;
}

static void puttile(Minefield *f, chtype ch, int mark) {
	NC *nc = (NC *) f->scr->data;
	char tile = ch & A_CHARTEXT;
	if (nc == NULL) {
		putchar(tile);
		return;
	}
	WINDOW *w = nc->field;
	if (tile == '/') {
		ch = ACS_DIAMOND;
	} else if (tile == '.') {
		ch = ACS_BULLET;
	}
	if (mark == 1) {
		ch |= COLOR_PAIR(PAIR_PRIMARY_MARK);
	} else if (mark == 2) {
		ch |= COLOR_PAIR(PAIR_SECONDARY_MARK);
	} else if (tile == '/') {
		ch |= COLOR_PAIR(PAIR_FLAG);
	} else if (tile == '.') {
		ch |= COLOR_PAIR(PAIR_UNKNOWN);
	} else if (tile == '@') {
		ch |= COLOR_PAIR(PAIR_BOMB);
	} else if (tile == ' ') {
		ch |= COLOR_PAIR(PAIR_VOID);
	} else {
		ch |= COLOR_PAIR(PAIR_WALL);
	}
	waddch(w, ch);
}

static void updatefield(Minefield *f, const char *field) {
	NC *nc = (NC *) f->scr->data;
	if (nc == NULL) {
		printf("%s", field);
		return;
	}
	WINDOW *w = nc->field;
	const int lineoffset = f->outputwidth+1;
	const chtype lines[] = {
		/* 0bABCD: A : above, B : right, C: below, D: left */
		/* 00          01         10          11 */
		ACS_BULLET, ACS_HLINE, ACS_VLINE, ACS_URCORNER, /* 00xx */
		ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_TTEE, /* 01xx */
		ACS_VLINE, ACS_LRCORNER, ACS_VLINE, ACS_RTEE, /* 10xx */
		ACS_LLCORNER, ACS_BTEE, ACS_LTEE, ACS_PLUS /* 11xx */
	};
	if (nc->colors) {
		wmove(w, 0, 0);
		int x = 0;
		int y = 0;
		char left = '\0';
		char right = *field;
		while (right != '\0') {
			char ch = right;
			right = *++field;
			if (ch == '+') {
				char above = y ? *(field-lineoffset-1) : '+';
				char below = (y+1 < f->outputheight) ? *(field+lineoffset-1) : '+';
				waddch(w, lines[(left == '+' || !x)
				                + ((below == '+') << 1)
				                + ((right == '+' || right == '\n' || right == '\0') << 2)
				                + ((above == '+') << 3)] | COLOR_PAIR(PAIR_BOUNDS));
			} else if (ch == '\n') {
				x = -1;
				++y;
				waddch(w, '\n');
				ch = '\0';
			} else {
				puttile(f, ch, 0);
			}
			left = ch;
			++x;
		}
		return;
	}
	mvwprintw(w, 0, 0, "%s", field);
	wrefresh(w);
}

static void updatetile_mark(Minefield *f, int idx, int mark) {
	NC *nc = (NC *) f->scr->data;
	if (nc == NULL) {
		return;
	}
	int row = outputrow(f, idxtocoords(f, idx));
	int column = outputcolumn(f, idxtocoords(f, idx));
	char c = tilechar(f->tiles+idx);
	WINDOW *w = nc->field;
	wmove(w, row, column);
	puttile(f, c, mark);
	wrefresh(w);
}

static void updatetile(Minefield *f, int idx) {
	updatetile_mark(f, idx, 0);
}

static void speak(Minefield *f, const char *fmt, ...) {
	va_list argp;
	va_start(argp, fmt);
	NC *nc = (NC *) f->scr->data;
	if (nc == NULL) {
		vprintf(fmt, argp);
		va_end(argp);
		return;
	}
	WINDOW *s = nc->speak;
	vwprintw(s, fmt, argp);
	va_end(argp);
	wrefresh(s);
}

static void ncmark(Minefield *f, int idx, int mark) {
	NC *nc = (NC *) f->scr->data;
	NCmark *add = new NCmark;
	add->idx = idx;
	add->mark = mark;
	add->next = nc->mark;
	nc->mark = add;
	updatetile_mark(f, idx, mark);
}

static void ncresetmarks(Minefield *f) {
	NC *nc = (NC *) f->scr->data;
	NCmark *m = nc->mark;
	while (m != NULL) {
		updatetile_mark(f, m->idx, 0);
		m = m->next;
	}
	freemarks(nc);
}

void ncscreen(Screen *s, Minefield *f) {
	silentscreen(s, f);
	s->init = &screeninit;
	s->deinit = &screendeinit;
	s->updatefield = &updatefield;
	s->updatetile = &updatetile;
	s->speak = &speak;
	s->mark = &ncmark;
	s->resetmarks = &ncresetmarks;
	s->data = NULL;
}
