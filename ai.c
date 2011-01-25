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

static bool issubset(int *superset, int *subset) {
	while (*subset != -1) {
		int *p = superset;
		while (*p != -1) {
			if (*p == *subset) return 0;
			++p;
		}
		++subset;
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
	while (tiles[count] != -1) {
		int idx = tiles[count++];
		printtile(f, idx);
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
		int i = 0;
		while (an[i] != -1) {
			if (f->tiles[an[i++]].flags & TILE_FLAGGED) {
				--anb;
			}
		}
	}

	// get a's unknown neighbourhood (unflagged, unpressed)
	int anu[f->maxneighbours];
	{ int *a = an; int *b = anu; while ((*b++ = *a++) != -1); }
	neighbourfilter(f, anu, &neighbourunknown_cb, NULL);

	{
		int i = 0;
		while (an[i] != -1) {
			int bidx = an[i++];
			Tile *b = &f->tiles[bidx];

			if (!(b->flags & TILE_PRESSED)) continue;

			// get b's neighbourhood
			int bn[f->maxneighbours];
			neighbourhood(f, idx, (int *) bn);

			// get b's bomb neighbour count minus already flagged bombs
			int bnb = b->neighbours;
			{
				int i = 0;
				while (bn[i] != -1) {
					if (f->tiles[bn[i++]].flags & TILE_FLAGGED) {
						--bnb;
					}
				}
			}

			// get b's unknown neighbourhood (unflagged, unpressed)
			int bnu[f->maxneighbours];
			{ int *a = bn; int *b = bnu; while ((*b++ = *a++) != -1); }
			neighbourfilter(f, bnu, &neighbourunknown_cb, NULL);

			if (!issubset(bnu, anu)) continue;

			neighbourfilter(f, bnu, &neighbourdifference_cb, anu);

			int count = 0; while (bnu[count] != -1) ++count;
			if (!count) continue;

			Action act;
			if (bnb == anb) {
				act.type = PRESS;
			} else if (count == bnb-anb) {
				act.type = FLAG;
			} else {
				continue;
			}
			Action **res = (Action **) malloc(sizeof(Action *)*(count+1));
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
