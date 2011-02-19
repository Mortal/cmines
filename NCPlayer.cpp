#include "types.h"
#include "NCPlayer.h"
#include "NCScreen.h"
#include <ncurses.h>
#include <stdlib.h>

#define GETSCR(scr) WINDOW *scr = ((class NCScreen *) this->f->scr)->getField()

const char inckeys[] = "dslk";
const char deckeys[] = "awji";

void NCPlayer::setcursor() {
	NCply *d = this->payload;
	int idx = d->cursidx;
	this->f->resetmarks();
	this->f->mark(idx, 1);
	int *neighbours = this->f->neighbourhood(idx);
	int i;
	for (i = 0; i < this->f->maxneighbours; ++i) {
		int n = neighbours[i];
		if (n == -1) continue;
		this->f->mark(n, 2);
	}
	this->f->neighbourhood_free(neighbours);
}

Action **NCPlayer::act() {
	//GETSCR(scr);
	NCply *d = (NCply *) this->payload;
	Action *act = new Action;
	act->type = NOOP;
	Action **res = new Action*[2];
	res[0] = act;
	res[1] = NULL;
	int ch = getch();
	if (ch == 'q') {
		act->type = GIVEUP;
	} else if (ch == 'f') {
		act->type = FLAG;
		act->tileidx = d->cursidx;
	} else if (ch == 'p') {
		act->type = PRESS;
		act->tileidx = d->cursidx;
	} else if (ch == 'z') {
		this->f->tiles[d->cursidx].flags ^= TILE_MINE;
		this->f->recalcneighbours();
		this->f->redrawfield();
	} else {
		int i;
		for (i = 0; inckeys[i] != '\0' && deckeys[i] != '\0' && i < this->f->dimcount; ++i) {
			if (ch != inckeys[i] && ch != deckeys[i]) {
				continue;
			}
			int delta = this->f->dimensionproducts[this->f->dimcount-i-1];
			if (ch == deckeys[i]) delta = -delta;
			if (-delta > d->cursidx) d->cursidx += this->f->tilecount;
			d->cursidx += delta;
			d->cursidx %= this->f->tilecount;
		}
	}
	this->setcursor();
	return res;
}

void NCPlayer::free(Action **a) {
	int i = 0;
	while (a[i] != NULL) {
		delete a[i++];
	}
	delete a;
}

NCPlayer::NCPlayer(Minefield *f) {
	this->f = f;
	cbreak(); // TODO: use raw() instead
	noecho();
	NCply *n;
	this->payload = n = new NCply;
	n->cursidx = 0;
}

NCPlayer::~NCPlayer() {
	delete this->payload;
}
