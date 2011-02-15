#include "types.h"
#include "ncplayer.h"
#include "ncscreen.h"
#include <ncurses.h>
#include <stdlib.h>

#define GETSCR(f, scr) WINDOW *scr = ((struct NCscreen *) f->scr->data)->field

const char inckeys[] = "dslk";
const char deckeys[] = "awji";

void NCPlayer::setcursor(Minefield *f, WINDOW *scr) {
	NCply *d = this->payload;
	int idx = d->cursidx;
	f->scr->resetmarks(f);
	f->scr->mark(f, idx, 1);
	int neighbours[f->maxneighbours];
	f->neighbourhood(idx, neighbours);
	int i;
	for (i = 0; i < f->maxneighbours; ++i) {
		int n = neighbours[i];
		if (n == -1) continue;
		f->scr->mark(f, n, 2);
	}
	wrefresh(scr);
}

Action **NCPlayer::act(Minefield *f) {
	GETSCR(f, scr);
	NCply *d = (NCply *) this->payload;
	Action *act = new Action;
	Action **res = new Action*[2];
	res[0] = act;
	res[1] = NULL;
	while (1) {
		this->setcursor(f, scr);
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
			f->recalcneighbours();
			f->printfield();
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

void NCPlayer::free(Action **a) {
	int i = 0;
	while (a[i] != NULL) {
		delete a[i++];
	}
	delete a;
}

void NCPlayer::init(Minefield *f) {
	cbreak(); // TODO: use raw() instead
	noecho();
	NCply *n;
	this->payload = n = new NCply;
	n->cursidx = 0;
}

void NCPlayer::deinit(Minefield *f) {
	delete this->payload;
}

NCPlayer::NCPlayer(Minefield *f) {
	this->payload = NULL;
}
