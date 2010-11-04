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
	int i = 0;
	int matches = 0;
	while (neighbours[i] != -1) {
		int idx = neighbours[i++];
		Tile *tile = &f->tiles[idx];
		if ((*cb)(f, tile, idx, cbpayload)) {
			++matches;
		}
	}
	return matches;
}

static void neighbourfilter(Minefield *f, int *c, neighbourcount_cb cb, void *cbpayload) {
	int *dest = c;
	int i = 0;
	while (c[i] != -1) {
		int idx = c[i];
		Tile *tile = &f->tiles[idx];
		if ((*cb)(f, tile, idx, cbpayload)) {
			if (&c[i] != dest) {
				*dest = c[i];
			}
			++dest;
		}
		i++;
	}
	*dest = -1;
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
	int i = 0;
	int *set = (int *) payload;
	while (set[i] != -1) {
		if (set[i] == idx) return 0;
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
	int neighbours[f->maxneighbours];
	neighbourhood(f, idx, (int *) neighbours);
	int neighbourunknown = neighbourcount(f, (int *) neighbours, &neighbourunknown_cb, NULL);
	if (!neighbourunknown) return NULL;
	int neighbourflags = neighbourcount(f, (int *) neighbours, &neighbourflags_cb, NULL);
	if (tile->neighbours != neighbourunknown + neighbourflags) return NULL;

	Action **ret = malloc(sizeof(Action *)*(neighbourunknown+1));
	int retidx = 0;
	int i = 0;
	while (neighbours[i] != -1) {
		int idx = neighbours[i++];
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
	int i = 0;
	while (neighbours[i] != -1) {
		int idx = neighbours[i++];
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
	 * Neighbourhoods, all: an, bn
	 * Neighbourhoods, bomb neighbours: anb, bnb
	 * Neighbourhoods, unpressed: anu, bnu
	 * HTH
	 */
	int an[f->maxneighbours];
	neighbourhood(f, idx, (int *) an);
	int anu[f->maxneighbours];
	{ int *a = an; int *b = anu; while ((*b++ = *a++) != -1); }
	neighbourfilter(f, anu, &neighbourunpressed_cb, NULL);
	{
		int i = 0;
		while (an[i] != -1) {
			int bidx = an[i++];
			Tile *b = &f->tiles[bidx];
			if (!(b->flags & TILE_PRESSED) || b->neighbours < a->neighbours) continue;
			/* B has as many as or more bomb neighbours than A */

			int bn[f->maxneighbours];
			neighbourhood(f, bidx, (int *) bn);

			int bnu[f->maxneighbours];
			{ int *a = bn; int *b = bnu; while ((*b++ = *a++) != -1); }
			neighbourfilter(f, bnu, &neighbourunpressed_cb, NULL);

			if (!issubset((void **) bnu, (void **) anu)) continue;
			/* A has no bomb neighbours that B doesn't have */

			neighbourfilter(f, bnu, &neighbourdifference_cb, anu);

			if (*bnu == -1) continue;
			/* B has unpressed neighbours that A doesn't have */
			int unflagged = neighbourcount(f, bnu, &neighbournoflags_cb, NULL);

			if (!unflagged) continue;

			int count = 0; while (bnu[count] != -1) ++count;
			if (b->neighbours-a->neighbours > count) continue;
			Action **res = (Action **) malloc(sizeof(Action *)*(count+1));
			Action act;
			act.type = (a->neighbours == b->neighbours) ? PRESS : FLAG;
			int i = 0;
			while (bnu[i] != -1) {
				act.tileidx = bnu[i];
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
