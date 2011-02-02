#include <stdio.h>
#include <stdlib.h>
#include "ai.h"
#include "Player.h"

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

typedef bool (*neighbourcount_cb)(Minefield *f, Tile *tile, int idx, void *payload);

static int neighbourcount(Minefield *f, int *neighbours, neighbourcount_cb cb, void *cbpayload) {
	int i;
	int matches = 0;
	for (i = 0; i < f->maxneighbours; ++i) {
		int idx = neighbours[i];
		if (idx == -1) continue;
		Tile *tile = &f->tiles[idx];
		if ((*cb)(f, tile, idx, cbpayload)) {
			++matches;
		}
	}
	return matches;
}

static void neighbourfilter(Minefield *f, int *c, neighbourcount_cb cb, void *cbpayload) {
	int dest = 0;
	int i;
	for (i = 0; i < f->maxneighbours; ++i) {
		int idx = c[i];
		if (idx == -1) continue;
		Tile *tile = &f->tiles[idx];
		if ((*cb)(f, tile, idx, cbpayload)) {
			if (i != dest) {
				c[dest] = c[i];
			}
			++dest;
		}
	}
	while (dest < f->maxneighbours) {
		c[dest++] = -1;
	}
}

#define CB(fun) static bool fun(Minefield *f, Tile *tile, int idx, void *payload)
CB(neighbourunpressed_cb) {
	return !(tile->flags & TILE_PRESSED);
}
CB(neighbourunknown_cb) {
	return !(tile->flags & (TILE_PRESSED|TILE_FLAGGED));
}
CB(neighbourflags_cb) {
	return !!(tile->flags & TILE_FLAGGED);
}
CB(neighbournoflags_cb) {
	return !(tile->flags & TILE_FLAGGED);
}
CB(neighbourneighbour_cb) {
	return (tile->flags & TILE_PRESSED) && (tile->neighbours > 0);
}
CB(neighbourdifference_cb) {
	int i;
	int *set = (int *) payload;
	for (i = 0; i < f->maxneighbours; ++i) {
		if (set[i] == idx) return 0;
	}
	return 1;
}
#undef CB

#define ACT(method) static Action **method(Minefield *f, int idx)
#define GETTILE(tile) Tile *tile = &f->tiles[idx]

ACT(act_dumb) {
	GETTILE(tile);
	if (tile->flags & (TILE_PRESSED|TILE_FLAGGED)) return NULL;

	Action **res = (Action **) malloc(sizeof(Action *)*2);

	Action act;
	act.type = PRESS;
	act.tileidx = idx;
	res[0] = (Action *) malloc(sizeof(Action));
	*res[0] = act;

	res[1] = NULL;

	return res;
}

ACT(act_singleflagging) {
	GETTILE(tile);
	if (!(tile->flags & (TILE_PRESSED|TILE_FLAGGED))) return NULL;
	if (!tile->neighbours) return NULL;
	int neighbours[f->maxneighbours];
	neighbourhood(f, idx, (int *) neighbours);
	int neighbourunknown = neighbourcount(f, (int *) neighbours, &neighbourunknown_cb, NULL);
	if (!neighbourunknown) return NULL;
	int neighbourflags = neighbourcount(f, (int *) neighbours, &neighbourflags_cb, NULL);
	if (tile->neighbours != neighbourunknown + neighbourflags) return NULL;

	Action **ret = malloc(sizeof(Action *)*(neighbourunknown+1));
	int retidx = 0;
	int i = 0;
	for (i = 0; i < f->maxneighbours; ++i) {
		int idx = neighbours[i];
		if (idx == -1) continue;
		Tile *t = &f->tiles[idx];
		if (!(t->flags & (TILE_PRESSED|TILE_FLAGGED))) {
			Action act;
			act.type = FLAG;
			act.tileidx = idx;
			Action *pact = ret[retidx++] = (Action *) malloc(sizeof(Action));
			*pact = act;
		}
	}
	ret[retidx] = NULL;
	return ret;
}

ACT(act_safespots) {
	GETTILE(tile);
	if (!(tile->flags & (TILE_PRESSED|TILE_FLAGGED))) return NULL;
	if (!tile->neighbours) return NULL;
	int neighbours[f->maxneighbours];
	neighbourhood(f, idx, (int *) neighbours);
	int neighbourunknown = neighbourcount(f, (int *) neighbours, &neighbourunknown_cb, NULL);
	if (!neighbourunknown) return NULL;
	int neighbourflags = neighbourcount(f, (int *) neighbours, &neighbourflags_cb, NULL);
	if (tile->neighbours != neighbourflags) return NULL;

	Action **ret = malloc(sizeof(Action *)*(neighbourunknown+1));
	int retidx = 0;
	int i;
	for (i = 0; i < f->maxneighbours; ++i) {
		int idx = neighbours[i];
		if (idx == -1) continue;
		Tile *t = &f->tiles[idx];
		if (!(t->flags & (TILE_PRESSED|TILE_FLAGGED))) {
			Action act;
			act.type = PRESS;
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
	Tile *t = &f->tiles[idx];
	Coordinate *coords = idxtocoords(f, idx);
	Dimension d;
	for (d = 0; d < f->dimcount; ++d) {
		printf("%d,", coords[d]);
	}
	printf(" neighbours=%d flags=%x\n", t->neighbours, t->flags);
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
	printf("%d tiles\n", count);
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
	neighbourfilter(f, anu, &neighbourunknown_cb, NULL);

	{
		int i;
		for (i = 0; i < f->maxneighbours; ++i) {
			int bidx = an[i];
			if (bidx == -1) continue;
			Tile *b = &f->tiles[bidx];

			if (!(b->flags & TILE_PRESSED)) continue;

#ifdef DEBUG
			bool debug = idx == 55*200+2 && bidx == 55*200+1;

			if (debug) {
				printtile(f, idx);
				printtile(f, bidx);
			}
#else
#  define debug (0)
#endif

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
						//if (debug) printtile(f, bn[i-1]);
						--bnb;
					}
				}
			}

			if (debug) printf("A has %d/%d neighbours and B has %d/%d neighbours\n", anb, a->neighbours, bnb, b->neighbours);

			// get b's unknown neighbourhood (unflagged, unpressed)
			int bnu[f->maxneighbours];
			{
				int i;
				for (i = 0; i < f->maxneighbours; ++i) {
					bnu[i] = bn[i];
				}
			}
			neighbourfilter(f, bnu, &neighbourunknown_cb, NULL);

			neighbourfilter(f, bnu, &neighbourdifference_cb, anu);

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
#define ACT(method) {Action **ret = method(f, idx); if (ret != NULL) {allowcoordreset = 1; return ret;}}
	while (hasnexttile(f)) {
		int idx = nexttileidx(f);
		ACT(act_singleflagging);
		ACT(act_safespots);
	}
	allowcoordreset = 1;
	while (hasnexttile(f)) {
		int idx = nexttileidx(f);
		ACT(act_dualcheck);
	}
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
