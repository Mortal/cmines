#include "types.h"
#include "ncplayer.h"
#include "ncscreen.h"
#include <ncurses.h>
#include <stdlib.h>

#define GETSCR(f, scr) WINDOW *scr = ((struct NCscreen *) f->scr->data)->field

typedef struct {
	int cursidx;
} NCply;

const char inckeys[] = "dslk";
const char deckeys[] = "awji";

static void setcursor(Player *p, Minefield *f, WINDOW *scr) {
	NCply *d = (NCply *) p->payload;
	int idx = d->cursidx;
	f->scr->resetmarks(f);
	f->scr->mark(f, idx, 1);
	int neighbours[f->maxneighbours];
	neighbourhood(f, idx, neighbours);
	int i;
	for (i = 0; i < f->maxneighbours; ++i) {
		int n = neighbours[i];
		if (n == -1) continue;
		f->scr->mark(f, n, 2);
	}
	wrefresh(scr);
}

static Action **ncact(Player *p, Minefield *f) {
	GETSCR(f, scr);
	NCply *d = (NCply *) p->payload;
	Action *act = (Action *) malloc(sizeof(Action));
	Action **res = (Action **) malloc(sizeof(Action *)*2);
	res[0] = act;
	res[1] = NULL;
	while (1) {
		setcursor(p, f, scr);
		int ch = getch();
		if (ch == 'q') {
			act->type = GIVEUP;
			return res;
		} else if (ch == 'f') {
			act->type = FLAG;
			act->tileidx = d->cursidx;
			return res;
		} else if (ch == 'p') {
			act->type = PRESS;
			act->tileidx = d->cursidx;
			return res;
		} else if (ch == 'z') {
			f->tiles[d->cursidx].flags ^= TILE_MINE;
			recalcneighbours(f);
			printfield(f);
		} else {
			int i;
			for (i = 0; inckeys[i] != '\0' && deckeys[i] != '\0' && i < f->dimcount; ++i) {
				if (ch != inckeys[i] && ch != deckeys[i]) {
					continue;
				}
				int delta = f->dimensionproducts[f->dimcount-i-1];
				if (ch == deckeys[i]) delta = -delta;
				if (-delta > d->cursidx) d->cursidx += f->tilecount;
				d->cursidx += delta;
				d->cursidx %= f->tilecount;
			}
		}
	}
}

static void ncfree(Player *p, Action **a) {
	int i = 0;
	while (a[i] != NULL) {
		free(a[i++]);
	}
	free(a);
}

static void ncinit(Player *p, Minefield *f) {
	cbreak(); // TODO: use raw() instead
	noecho();
	NCply *n = p->payload = (NCply *) malloc(sizeof(NCply));
	n->cursidx = 0;
}

static void ncdeinit(Player *p, Minefield *f) {
	free(p->payload);
}

void NCPlayer(Player *p, Minefield *f) {
	p->initfun = &ncinit;
	p->deinitfun = &ncdeinit;
	p->actfun = &ncact;
	p->freefun = &ncfree;
	p->payload = NULL;
}
