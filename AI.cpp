#include <stdio.h>
#include <stdlib.h>
#include "AI.h"
#include "Player.h"
#include "Screen.h"

void AI::giveup(Action *act) {
	act->type = GIVEUP;
}

bool AI::hasnexttile(Minefield *f) {
	return (this->allowcoordreset || this->nexttileidx_ < f->tilecount);
}

int AI::nexttileidx(Minefield *f) {
	if (this->nexttileidx_ < f->tilecount) {
		return this->nexttileidx_++;
	}
	this->allowcoordreset = 0;
	return this->nexttileidx_ = 0;
}

#define COUNTER(fun, cb) \
int AI::fun(Minefield *f, int *neighbours) {\
	int i;\
	int matches = 0;\
	for (i = 0; i < f->maxneighbours; ++i) {\
		int idx = neighbours[i];\
		if (idx == -1) continue;\
		Tile *tile = &f->tiles[idx];\
		if (cb) {\
			++matches;\
		}\
	}\
	return matches;\
}

#define FILTER(fun, cb) \
void AI::fun(Minefield *f, int *c) {\
	int dest = 0;\
	int i;\
	for (i = 0; i < f->maxneighbours; ++i) {\
		int idx = c[i];\
		if (idx == -1) continue;\
		Tile *tile = &f->tiles[idx];\
		if (cb) {\
			if (i != dest) {\
				c[dest] = c[i];\
			}\
			++dest;\
		}\
	}\
	while (dest < f->maxneighbours) {\
		c[dest++] = -1;\
	}\
}

FILTER(filterunknown, !(tile->flags & (TILE_PRESSED|TILE_FLAGGED)))
COUNTER(countunknown, !(tile->flags & (TILE_PRESSED|TILE_FLAGGED)))
COUNTER(countflags, !!(tile->flags & TILE_FLAGGED))
#undef CB

void AI::neighbourdifference(Minefield *f, int *c, int *set) {
	int i, j, k; /* i is read-index in c, j is write-index is c, k is read-index in set */
	int length = f->maxneighbours;
	for (i = 0, j = 0, k = 0; i < length; ++i) {
		int tofind = c[i];
		if (tofind == -1) continue;
		while (k < length && set[k] < tofind) ++k;
		if (set[k] == tofind) continue;
		if (i != j) {
			c[j] = c[i];
		}
		++j;
	}
	while (j < length) {
		c[j] = -1;
		++j;
	}
}

#define ACT(method) Action **AI::method(Minefield *f, int idx)
#define GETTILE(tile) Tile *tile = &f->tiles[idx]

ACT(act_singlecheck) {
	GETTILE(tile);
	if (!(tile->flags & TILE_PRESSED)) return NULL;
	if (!tile->neighbours) return NULL;
	int neighbours[f->maxneighbours];
	f->neighbourhood(idx, (int *) neighbours);
	int neighbourunknown = countunknown(f, (int *) neighbours);
	if (!neighbourunknown) return NULL;
	int neighbourflags = countflags(f, (int *) neighbours);
	Action act;
	if (tile->neighbours == neighbourflags) {
		act.type = PRESS;
	} else if (tile->neighbours == neighbourunknown + neighbourflags) {
		act.type = FLAG;
	} else {
		return NULL;
	}

	Action **ret = new Action*[neighbourunknown+1];
	int retidx = 0;
	int i = 0;
	for (i = 0; i < f->maxneighbours; ++i) {
		int idx = neighbours[i];
		if (idx == -1) continue;
		Tile *t = &f->tiles[idx];
		if (!(t->flags & (TILE_PRESSED|TILE_FLAGGED))) {
			act.tileidx = idx;
			Action *pact = ret[retidx++] = new Action;
			*pact = act;
		}
	}
	ret[retidx] = NULL;
	return ret;
}

bool AI::issubset(int *superset, int *subset, int length) {
	int i, j; /* i is index in subset, j in superset */
	for (i = 0, j = 0; i < length; ++i) {
		int tofind = subset[i];
		if (tofind == -1) continue;
		while (j < length && superset[j] == -1) ++j;
		if (j >= length || superset[j] != tofind) return 0;
	}
	return 1;
}


ACT(act_dualcheck) {
	GETTILE(a);
	/* Tiles: a, b
	 * Indices: idx, bidx
	 * Neighbourhoods, all: an, bn
	 * Neighbourhoods, bomb count: anb, bnb (the number of the tile minus the number of flagged neighbours)
	 * Neighbourhoods, unpressed, unflagged: anu, bnu
	 * HTH
	 */

	// get a's neighbourhood
	int an[f->maxneighbours];
	f->neighbourhood(idx, (int *) an);

	// get a's bomb neighbour count minus already flagged bombs
	int anb = a->neighbours;
	{
		int i;
		for (i = 0; i < f->maxneighbours; ++i) {
			if (an[i] == -1) continue;
			if (f->tiles[an[i]].flags & TILE_FLAGGED) {
				--anb;
			}
		}
	}

	// get a's unknown neighbourhood (unflagged, unpressed)
	int anu[f->maxneighbours];
	{
		int i;
		for (i = 0; i < f->maxneighbours; ++i) {
			anu[i] = an[i];
		}
	}
	filterunknown(f, anu);

	{
		int i;
		for (i = 0; i < f->maxneighbours; ++i) {
			int bidx = an[i];
			if (bidx == -1) continue;
			Tile *b = &f->tiles[bidx];

			if (!(b->flags & TILE_PRESSED)) continue;

			// get b's neighbourhood
			int bn[f->maxneighbours];
			f->neighbourhood(bidx, (int *) bn);

			// get b's bomb neighbour count minus already flagged bombs
			int bnb = b->neighbours;
			{
				int i;
				for (i = 0; i < f->maxneighbours; ++i) {
					if (bn[i] == -1) continue;
					if (f->tiles[bn[i]].flags & TILE_FLAGGED) {
						--bnb;
					}
				}
			}

			// get b's unknown neighbourhood (unflagged, unpressed)
			int bnu[f->maxneighbours];
			{
				int i;
				for (i = 0; i < f->maxneighbours; ++i) {
					bnu[i] = bn[i];
				}
			}
			filterunknown(f, bnu);

			neighbourdifference(f, bnu, anu);

			int count = 0;
			{
				int i;
				for (i = 0; i < f->maxneighbours; ++i) {
					if (bnu[i] != -1) ++count;
				}
			}
			if (!count) continue;

			Action act;
			if (count == bnb-anb) {
				act.type = FLAG;
			} else if (issubset(bnu, anu, f->maxneighbours) && bnb == anb) {
				act.type = PRESS;
			} else {
				continue;
			}
			Action **res = new Action*[count+1];
			int i, j = 0;
			for (i = 0; i < f->maxneighbours; ++i) {
				act.tileidx = bnu[i];
				if (act.tileidx == -1) continue;
				res[j] = new Action;
				*res[j] = act;
				j++;
			}
			res[j] = NULL;
			return res;
		}
	}
	return NULL;
}
#undef GETTILE
#undef ACT

Action **AI::act(Minefield *f) {
#define ACT(method) {\
	Action **ret = this->method(f, idx);\
	if (ret != NULL) {\
		/*\
		if (f->sleep) {\
			f->scr->resetmarks(f);\
			f->scr->mark(f, idx, 1);\
			int i;\
			for (i = 0; ret[i] != NULL; ++i) {\
				f->scr->mark(f, ret[i]->tileidx, 2);\
			}\
		}\
		*/\
		this->allowcoordreset = 1;\
		/*\
		char msg[256];\
		snprintf(msg, 255, "AI used %s\n", #method);\
		msg[255] = 0;\
		((Screen *) f->scr)->speak(f, msg);\
		printtile(f, idx);\
		*/\
		return ret;\
	}\
}
	// first, try the simple calculation on all tiles. act_singlecheck only calls
	// neighbourhood() once and some counting functions per call.
	while (this->hasnexttile(f)) {
		int idx = this->nexttileidx(f);
		ACT(act_singlecheck);
	}
	// once we've exhausted the playing field (run through from top to bottom
	// with no match), allowcoordreset is set to 0. reset it to 1 and try again
	// with the complex algorithm. act_dualcheck calls neighbourhood() once for
	// each tile and once again for each tile's pressed neighbours. this is a
	// slow operation! if we have a match, return it. next time, start over with
	// simple calculations.
	this->allowcoordreset = 1;
	while (this->hasnexttile(f)) {
		int idx = this->nexttileidx(f);
		ACT(act_dualcheck);
	}
	// we've exhausted the playing field twice now. we give up since the board is
	// ambiguous.
	Action **res = new Action*[2];
	res[0] = new Action;
	this->giveup(res[0]);
	res[1] = NULL;
	return res;
#undef ACT
}

void AI::free(Action **act) {
	int i = 0;
	while (act[i] != NULL) {
		delete act[i++];
	}
	delete act;
}

void AI::init(Minefield *f) {
}

void AI::deinit(Minefield *f) {
}

AI::AI(Minefield *f) {
	this->allowcoordreset = 0;
	this->nexttileidx_ = 0;
}
