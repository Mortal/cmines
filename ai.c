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

typedef bool (*neighbourcount_cb)(Minefield *f, Tile *tile, Coordinate *coord, void *payload);

static int neighbourcount(Minefield *f, Coordinate **neighbours, neighbourcount_cb cb, void *cbpayload) {
	int i = 0;
	int matches = 0;
	while (neighbours[i] != NULL) {
		Coordinate *coord = neighbours[i++];
		Tile *tile = &f->tiles[coordstoidx(f, coord)];
		if ((*cb)(f, tile, coord, cbpayload)) {
			++matches;
		}
	}
	return matches;
}

static void neighbourfilter(Minefield *f, Coordinate **c, neighbourcount_cb cb, void *cbpayload) {
	Coordinate **dest = c;
	int i = 0;
	while (c[i] != NULL) {
		Coordinate *coord = c[i++];
		Tile *tile = &f->tiles[coordstoidx(f, coord)];
		if ((*cb)(f, tile, coord, cbpayload)) {
			if (&c[i] != dest) {
				c[i] = *dest;
			}
			++dest;
		}
	}
	*dest = NULL;
}

#define CB(fun) static bool fun(Minefield *f, Tile *tile, Coordinate *coords, void *payload)
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
	int i = 0;
	int idx = coordstoidx(f, coords);
	Coordinate **set = (Coordinate **) payload;
	while (set[i] != NULL) {
		if (coordstoidx(f, set[i]) == idx) return 0;
		++i;
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
	Coordinate *neighbours[f->maxneighbours];
	neighbourhood(f, idx, (Coordinate **) neighbours);
	int neighbourunknown = neighbourcount(f, (Coordinate **) neighbours, &neighbourunknown_cb, NULL);
	if (!neighbourunknown) return NULL;
	int neighbourflags = neighbourcount(f, (Coordinate **) neighbours, &neighbourflags_cb, NULL);
	if (tile->neighbours != neighbourunknown + neighbourflags) return NULL;

	Action **ret = malloc(sizeof(Action *)*(neighbourunknown+1));
	int retidx = 0;
	int i = 0;
	while (neighbours[i] != NULL) {
		Coordinate *c = neighbours[i++];
		int idx = coordstoidx(f, c);
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
	Coordinate *neighbours[f->maxneighbours];
	neighbourhood(f, idx, (Coordinate **) neighbours);
	int neighbourunknown = neighbourcount(f, (Coordinate **) neighbours, &neighbourunknown_cb, NULL);
	if (!neighbourunknown) return NULL;
	int neighbourflags = neighbourcount(f, (Coordinate **) neighbours, &neighbourflags_cb, NULL);
	if (tile->neighbours != neighbourflags) return NULL;

	Action **ret = malloc(sizeof(Action *)*(neighbourunknown+1));
	int retidx = 0;
	int i = 0;
	while (neighbours[i] != NULL) {
		Coordinate *c = neighbours[i++];
		int idx = coordstoidx(f, c);
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

static bool issubset(void **superset, void **subset) {
	while (*subset != NULL) {
		void **p = superset;
		while (*p != NULL) {
			if (*p == *subset) return 0;
			++p;
		}
		++subset;
	}
	return 1;
}

ACT(act_dualcheck) {
	GETTILE(a);
	/* Tiles: a, b
	 * Indices: idx, bidx
	 * Coordinates: bcoord
	 * Neighbourhoods, all: an, bn
	 * Neighbourhoods, bomb neighbours: anb, bnb
	 * Neighbourhoods, unpressed: anu, bnu
	 * HTH
	 */
	Coordinate *an[f->maxneighbours];
	neighbourhood(f, idx, (Coordinate **) an);
	Coordinate *anu[f->maxneighbours];
	{ Coordinate **a = an; Coordinate **b = anu; while ((*a++ = *b++)); }
	neighbourfilter(f, anu, *neighbourunpressed_cb, NULL);
	{
		int i = 0;
		while (an[i] != NULL) {
			Coordinate *bcoord = an[i++];
			int bidx = coordstoidx(f, bcoord);
			Tile *b = &f->tiles[bidx];
			if (!(b->flags & TILE_PRESSED) || b->neighbours < a->neighbours) continue;
			/* B has as many as or more bomb neighbours than A */

			Coordinate *bn[f->maxneighbours];
			neighbourhood(f, bidx, (Coordinate **) bn);

			Coordinate *bnu[f->maxneighbours];
			{ Coordinate **a = bn; Coordinate **b = bnu; while ((*a++ = *b++)); }
			neighbourfilter(f, bnu, &neighbourunpressed_cb, NULL);

			if (!issubset((void **) bnu, (void **) anu)) continue;
			/* A has no bomb neighbours that B doesn't have */

			neighbourfilter(f, bnu, &neighbourdifference_cb, anu);

			if (*bnu == NULL) continue;
			/* B has unpressed neighbours that A doesn't have */
			int unflagged = neighbourcount(f, bnu, &neighbournoflags_cb, NULL);

			if (!unflagged) continue;

			int count = 0; while (bnu[count] != NULL); ++count;
			if (b->neighbours-a->neighbours > count) continue;
			Action **res = (Action **) malloc(sizeof(Action *)*(count+1));
			Action act;
			act.type = (a->neighbours == b->neighbours) ? PRESS : FLAG;
			int i = 0;
			while (bnu[i] != NULL) {
				Coordinate *coord = bnu[i];
				act.tileidx = coordstoidx(f, coord);
				res[i] = (Action *) malloc(sizeof(Action));
				*res[i] = act;
				i++;
			}
			res[i] = NULL;
			return res;
		}
	}
	return NULL;
}
#undef GETTILE
#undef ACT

static Action **act(Minefield *f) {
#define ACT(method) {Action **ret = method(f, idx); if (ret != NULL) {allowcoordreset = 1; return ret;}}
	while (hasnexttile(f)) {
		int idx = nexttileidx(f);
		//ACT(act_dumb);
		ACT(act_singleflagging);
		ACT(act_safespots);
	}
	Action **res = (Action **) malloc(sizeof(Action *)*2);
	res[0] = (Action *) malloc(sizeof(Action));
	giveup(res[0]);
	res[1] = NULL;
	return res;
#undef ACT
}

static void actfree(Action **act) {
	int i = 0;
	while (act[i] != NULL) {
		free(act[i++]);
	}
	free(act);
}

void AI(Player *ply) {
	Player ai = {&act, &actfree};
	*ply = ai;
}
