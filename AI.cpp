#include <stdio.h>
#include <stdlib.h>
#include "AI.h"
#include "Player.h"
#include "Screen.h"

void AI::giveup(Action *act) {
	act->type = GIVEUP;
}

bool AI::hasnexttile() {
	return (this->allowcoordreset || this->nexttileidx_ < this->f->tilecount);
}

int AI::nexttileidx() {
	if (this->nexttileidx_ < this->f->tilecount) {
		return this->nexttileidx_++;
	}
	this->allowcoordreset = 0;
	return this->nexttileidx_ = 0;
}

template <typename CB>
int AI::counter(Minefield * f, int *neighbours, CB * cb) {
	int i;
	int matches = 0;
	for (i = 0; i < f->maxneighbours; ++i) {
		int idx = neighbours[i];
		if (idx == -1) continue;
		const Tile & tile = f->tiles[idx];
		if (cb(tile)) {
			++matches;
		}
	}
	return matches;
}

template <typename CB>
void AI::filter(Minefield * f, int * neighbours, CB * cb) {
	int dest = 0;
	int i;
	for (i = 0; i < f->maxneighbours; ++i) {
		int idx = neighbours[i];
		if (idx == -1) continue;
		const Tile & tile = f->tiles[idx];
		if (cb(tile)) {
			if (i != dest) {
				neighbours[dest] = neighbours[i];
			}
			++dest;
		}
	}
	while (dest < f->maxneighbours) {
		neighbours[dest++] = -1;
	}
}

static bool countunknown_cb(const Tile & tile) {
	return !(tile.flags & (TILE_PRESSED|TILE_FLAGGED));
}

static bool countflags_cb(const Tile & tile) {
	return !!(tile.flags & TILE_FLAGGED);
}

int AI::countunknown(int *neighbours) {
	return counter(this->f, neighbours, &countunknown_cb);
}

int AI::countflags(int *neighbours) {
	return counter(this->f, neighbours, &countflags_cb);
}

static bool filterunknown_cb(const Tile & tile) {
	return !(tile.flags & (TILE_PRESSED|TILE_FLAGGED));
}

void AI::filterunknown(int * neighbours) {
	filter(this->f, neighbours, &filterunknown_cb);
}

void AI::neighbourdifference(int *c, int *set) {
	int i, j, k; /* i is read-index in c, j is write-index is c, k is read-index in set */
	int length = this->f->maxneighbours;
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

#define ACT(method) Action **AI::method(int idx)
#define GETTILE(tile) Tile *tile = &this->f->tiles[idx]

ACT(act_singlecheck) {
	GETTILE(tile);
	if (!(tile->flags & TILE_PRESSED)) return NULL;
	if (!tile->neighbours) return NULL;
	int *neighbours = this->f->neighbourhood(idx);
	int neighbourunknown = countunknown((int *) neighbours);
	if (!neighbourunknown) return NULL;
	int neighbourflags = countflags((int *) neighbours);
	Action act;
	if (tile->neighbours == neighbourflags) {
		act.type = PRESS;
	} else if (tile->neighbours == neighbourunknown + neighbourflags) {
		act.type = FLAG;
	} else {
		this->f->neighbourhood_free(neighbours);
		return NULL;
	}

	Action **ret = new Action*[neighbourunknown+1];
	int retidx = 0;
	int i = 0;
	for (i = 0; i < this->f->maxneighbours; ++i) {
		int idx = neighbours[i];
		if (idx == -1) continue;
		Tile *t = &this->f->tiles[idx];
		if (!(t->flags & (TILE_PRESSED|TILE_FLAGGED))) {
			act.tileidx = idx;
			Action *pact = ret[retidx++] = new Action;
			*pact = act;
		}
	}
	ret[retidx] = NULL;
	this->f->neighbourhood_free(neighbours);
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
	int *an = this->f->neighbourhood(idx);

	// get a's bomb neighbour count minus already flagged bombs
	int anb = a->neighbours;
	{
		int i;
		for (i = 0; i < this->f->maxneighbours; ++i) {
			if (an[i] == -1) continue;
			if (this->f->tiles[an[i]].flags & TILE_FLAGGED) {
				--anb;
			}
		}
	}

	// get a's unknown neighbourhood (unflagged, unpressed)
	int anu[this->f->maxneighbours];
	{
		int i;
		for (i = 0; i < this->f->maxneighbours; ++i) {
			anu[i] = an[i];
		}
	}
	filterunknown(anu);

	{
		int i;
		int *bn = NULL;
		for (i = 0; i < this->f->maxneighbours; ++i) {
			if (bn != NULL) {
				this->f->neighbourhood_free(bn);
				bn = NULL;
			}

			int bidx = an[i];
			if (bidx == -1) continue;
			Tile *b = &this->f->tiles[bidx];

			if (!(b->flags & TILE_PRESSED)) continue;

			// get b's neighbourhood
			bn = this->f->neighbourhood(bidx);

			// get b's bomb neighbour count minus already flagged bombs
			int bnb = b->neighbours;
			{
				int i;
				for (i = 0; i < this->f->maxneighbours; ++i) {
					if (bn[i] == -1) continue;
					if (this->f->tiles[bn[i]].flags & TILE_FLAGGED) {
						--bnb;
					}
				}
			}

			// get b's unknown neighbourhood (unflagged, unpressed)
			int bnu[this->f->maxneighbours];
			{
				int i;
				for (i = 0; i < this->f->maxneighbours; ++i) {
					bnu[i] = bn[i];
				}
			}
			filterunknown(bnu);

			neighbourdifference(bnu, anu);

			int count = 0;
			{
				int i;
				for (i = 0; i < this->f->maxneighbours; ++i) {
					if (bnu[i] != -1) ++count;
				}
			}
			if (!count) continue;

			Action act;
			if (count == bnb-anb) {
				act.type = FLAG;
			} else if (issubset(bnu, anu, this->f->maxneighbours) && bnb == anb) {
				act.type = PRESS;
			} else {
				continue;
			}
			Action **res = new Action*[count+1];
			int i, j = 0;
			for (i = 0; i < this->f->maxneighbours; ++i) {
				act.tileidx = bnu[i];
				if (act.tileidx == -1) continue;
				res[j] = new Action;
				*res[j] = act;
				j++;
			}
			res[j] = NULL;
			this->f->neighbourhood_free(an);
			this->f->neighbourhood_free(bn);
			return res;
		}
		if (bn != NULL) {
			this->f->neighbourhood_free(bn);
			bn = NULL;
		}
	}
	this->f->neighbourhood_free(an);
	return NULL;
}
#undef GETTILE
#undef ACT

Action **AI::act() {
#define ACT(method) {\
	Action **ret = this->method(idx);\
	if (ret != NULL) {\
		if (this->f->sleep) {\
			this->f->resetmarks();\
			this->f->mark(idx, 1);\
			int i;\
			for (i = 0; ret[i] != NULL; ++i) {\
				this->f->mark(ret[i]->tileidx, 2);\
			}\
		}\
		this->allowcoordreset = 1;\
		/*\
		char msg[256];\
		snprintf(msg, 255, "AI used %s\n", #method);\
		msg[255] = 0;\
		((Screen *) this->f->scr)->speak(this->f, msg);\
		printtile(this->f, idx);\
		*/\
		return ret;\
	}\
}
	// first, try the simple calculation on all tiles. act_singlecheck only calls
	// neighbourhood() once and some counting functions per call.
	while (this->hasnexttile()) {
		int idx = this->nexttileidx();
		ACT(act_singlecheck);
	}
	// once we've exhausted the playing field (run through from top to bottom
	// with no match), allowcoordreset is set to 0. reset it to 1 and try again
	// with the complex algorithm. act_dualcheck calls neighbourhood() once for
	// each tile and once again for each tile's pressed neighbours. this is a
	// slow operation! if we have a match, return it. next time, start over with
	// simple calculations.
	this->allowcoordreset = 1;
	while (this->hasnexttile()) {
		int idx = this->nexttileidx();
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

AI::AI(Minefield *f) {
	this->f = f;
	this->allowcoordreset = 0;
	this->nexttileidx_ = 0;
}
