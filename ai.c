#include <stdio.h>
#include <stdlib.h>
#include "ai.h"
#include "Player.h"
#include "Screen.h"

bool allowcoordreset = 0;
int nexttileidx_ = 0;

static void giveup(Action *act) {
	act->type = GIVEUP;
}

static bool hasnexttile(Minefield *f) {
	return (allowcoordreset || nexttileidx_ < f->tilecount);
}

static int nexttileidx(Minefield *f) {
	if (nexttileidx_ < f->tilecount) {
		return nexttileidx_++;
	}
	allowcoordreset = 0;
	return nexttileidx_ = 0;
}

#define COUNTER(fun, cb) \
static int fun(Minefield *f, int *neighbours) {\
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
static void fun(Minefield *f, int *c) {\
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

static void neighbourdifference(Minefield *f, int *c, int *set) {
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

#define ACT(method) static Action **method(Minefield *f, int idx)
#define GETTILE(tile) Tile *tile = &f->tiles[idx]

ACT(act_singlecheck) {
	GETTILE(tile);
	if (!(tile->flags & TILE_PRESSED)) return NULL;
	if (!tile->neighbours) return NULL;
	int neighbours[f->maxneighbours];
	neighbourhood(f, idx, (int *) neighbours);
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

	Action **ret = malloc(sizeof(Action *)*(neighbourunknown+1));
	int retidx = 0;
	int i = 0;
	for (i = 0; i < f->maxneighbours; ++i) {
		int idx = neighbours[i];
		if (idx == -1) continue;
		Tile *t = &f->tiles[idx];
		if (!(t->flags & (TILE_PRESSED|TILE_FLAGGED))) {
			act.tileidx = idx;
			Action *pact = ret[retidx++] = (Action *) malloc(sizeof(Action));
			*pact = act;
		}
	}
	ret[retidx] = NULL;
	return ret;
}

static bool issubset(int *superset, int *subset, int length) {
	int i, j; /* i is index in subset, j in superset */
	for (i = 0, j = 0; i < length; ++i) {
		int tofind = subset[i];
		if (tofind == -1) continue;
		while (j < length && superset[j] == -1) ++j;
		if (j >= length || superset[j] != tofind) return 0;
	}
	return 1;
}

#ifdef DEBUG
/* aid in debugging with gdb */
void printtile(Minefield *f, int idx) {
	Screen *s = (Screen *) f->scr;
	const int l = 512;
	char msg[l];
	int i = 0;
	Tile *t = &f->tiles[idx];
	Coordinate *coords = idxtocoords(f, idx);
	Dimension d;
	for (d = 0; d < f->dimcount; ++d) {
		i += snprintf(msg+i, l-i, "%d,", coords[d]); if (i >= l) break;
	}
	msg[l-1] = 0;
	s->speak(f, "%s neighbours=%d flags=%x\n", msg, t->neighbours, t->flags);
}

void printtiles(Minefield *f, int *tiles) {
	int count = 0;
	int i;
	for (i = 0; i < f->maxneighbours; ++i) {
		if (tiles[i] == -1) continue;
		int idx = tiles[i];
		printtile(f, idx);
		++count;
	}
	//printf("%d tiles\n", count);
}
#else
void printtile(Minefield *f, int idx) {
}
void printtiles(Minefield *f, int *tiles) {
}
#endif

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
	neighbourhood(f, idx, (int *) an);

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
			neighbourhood(f, bidx, (int *) bn);

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
			Action **res = (Action **) malloc(sizeof(Action *)*(count+1));
			int i, j = 0;
			for (i = 0; i < f->maxneighbours; ++i) {
				act.tileidx = bnu[i];
				if (act.tileidx == -1) continue;
				res[j] = (Action *) malloc(sizeof(Action));
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

static Action **act(Player *p, Minefield *f) {
#define ACT(method) {\
	Action **ret = method(f, idx);\
	if (ret != NULL) {\
		if (f->sleep) {\
			f->scr->resetmarks(f);\
			f->scr->mark(f, idx, 1);\
			int i;\
			for (i = 0; ret[i] != NULL; ++i) {\
				f->scr->mark(f, ret[i]->tileidx, 2);\
			}\
		}\
		allowcoordreset = 1;\
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
	while (hasnexttile(f)) {
		int idx = nexttileidx(f);
		ACT(act_singlecheck);
	}
	// once we've exhausted the playing field (run through from top to bottom
	// with no match), allowcoordreset is set to 0. reset it to 1 and try again
	// with the complex algorithm. act_dualcheck calls neighbourhood() once for
	// each tile and once again for each tile's pressed neighbours. this is a
	// slow operation! if we have a match, return it. next time, start over with
	// simple calculations.
	allowcoordreset = 1;
	while (hasnexttile(f)) {
		int idx = nexttileidx(f);
		ACT(act_dualcheck);
	}
	// we've exhausted the playing field twice now. we give up since the board is
	// ambiguous.
	Action **res = (Action **) malloc(sizeof(Action *)*2);
	res[0] = (Action *) malloc(sizeof(Action));
	giveup(res[0]);
	res[1] = NULL;
	return res;
#undef ACT
}

static void actfree(Player *p, Action **act) {
	int i = 0;
	while (act[i] != NULL) {
		free(act[i++]);
	}
	free(act);
}

static void aiinit(Player *p, Minefield *f) {
}

static void aideinit(Player *p, Minefield *f) {
}

void AI(Player *ply, Minefield *f) {
	Player ai = {&aiinit, &aideinit, &act, &actfree, NULL};
	*ply = ai;
	allowcoordreset = 0;
	nexttileidx_ = 0;
}
