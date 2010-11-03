#include <stdio.h>
#include <stdlib.h>
#include "ai.h"
#include "Player.h"

bool allowcoordreset = 0;
int nexttileidx_ = 0;

static void giveup(struct Action *act) {
	act->type = GIVEUP;
}

static bool hasnexttile(struct Minefield *f) {
	return (allowcoordreset || nexttileidx_ < f->tilecount);
}

static int nexttileidx(struct Minefield *f) {
	if (nexttileidx_ < f->tilecount) {
		return nexttileidx_++;
	}
	allowcoordreset = 0;
	return nexttileidx_ = 0;
}

typedef bool (*neighbourcount_cb)(struct Tile *tile, void *payload);

static int neighbourcount(struct Minefield *f, int tile, neighbourcount_cb cb, void *cbpayload) {
	Coordinate *neighbours[f->maxneighbours];
	neighbourhood(f, tile, (Coordinate **) neighbours);
	int i = 0;
	int matches = 0;
	while (neighbours[i] != NULL) {
		Coordinate *coord = neighbours[i++];
		struct Tile *tile = &f->tiles[coordstoidx(f, coord)];
		if ((*cb)(tile, cbpayload)) {
			++matches;
		}
	}
	return matches;
}

#define CB(fun) static bool fun(struct Tile *tile, void *payload)
CB(neighbourunknown_cb) {
	return !(tile->flags & (TILE_PRESSED|TILE_FLAGGED));
}
CB(neighbourflags_cb) {
	return !!(tile->flags & TILE_FLAGGED);
}
CB(neighbourneighbour_cb) {
	return (tile->flags & TILE_PRESSED) && (tile->neighbours > 0);
}
#undef CB

#define ACT(method) static struct Action **method(struct Minefield *f, int idx)
#define GETTILE(tile) struct Tile *tile = &f->tiles[idx]

ACT(act_dumb) {
	GETTILE(tile);
	if (tile->flags & (TILE_PRESSED|TILE_FLAGGED)) return NULL;

	struct Action **res = (struct Action **) malloc(sizeof(struct Action *)*2);

	struct Action act;
	act.type = PRESS;
	act.tileidx = idx;
	res[0] = (struct Action *) malloc(sizeof(struct Action));
	*res[0] = act;

	res[1] = NULL;

	return res;
}

ACT(act_singleflagging) {
	GETTILE(tile);
	if (!(tile->flags & (TILE_PRESSED|TILE_FLAGGED))) return NULL;
	if (!tile->neighbours) return NULL;
	int neighbourunknown = neighbourcount(f, idx, &neighbourunknown_cb, NULL);
	if (!neighbourunknown) return NULL;
	int neighbourflags = neighbourcount(f, idx, &neighbourflags_cb, NULL);
	if (tile->neighbours != neighbourunknown + neighbourflags) return NULL;

	struct Action **ret = malloc(sizeof(struct Action *)*(neighbourunknown+1));
	int retidx = 0;
	Coordinate *neighbours[f->maxneighbours];
	neighbourhood(f, idx, (Coordinate **) neighbours);
	int i = 0;
	while (neighbours[i] != NULL) {
		Coordinate *c = neighbours[i++];
		int idx = coordstoidx(f, c);
		struct Tile *t = &f->tiles[idx];
		if (!(t->flags & (TILE_PRESSED|TILE_FLAGGED))) {
			struct Action act;
			act.type = FLAG;
			act.tileidx = idx;
			struct Action *pact = ret[retidx++] = (struct Action *) malloc(sizeof(struct Action));
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
	int neighbourunknown = neighbourcount(f, idx, &neighbourunknown_cb, NULL);
	if (!neighbourunknown) return NULL;
	int neighbourflags = neighbourcount(f, idx, &neighbourflags_cb, NULL);
	if (tile->neighbours != neighbourflags) return NULL;

	struct Action **ret = malloc(sizeof(struct Action *)*(neighbourunknown+1));
	int retidx = 0;
	Coordinate *neighbours[f->maxneighbours];
	neighbourhood(f, idx, (Coordinate **) neighbours);
	int i = 0;
	while (neighbours[i] != NULL) {
		Coordinate *c = neighbours[i++];
		int idx = coordstoidx(f, c);
		struct Tile *t = &f->tiles[idx];
		if (!(t->flags & (TILE_PRESSED|TILE_FLAGGED))) {
			struct Action act;
			act.type = PRESS;
			act.tileidx = idx;
			struct Action *pact = ret[retidx++] = (struct Action *) malloc(sizeof(struct Action));
			*pact = act;
		}
	}
	ret[retidx] = NULL;
	return ret;
}
#undef GETTILE
#undef ACT

static struct Action **act(struct Minefield *f) {
#define ACT(method) {struct Action **ret = method(f, idx); if (ret != NULL) {allowcoordreset = 1; return ret;}}
	while (hasnexttile(f)) {
		int idx = nexttileidx(f);
		//ACT(act_dumb);
		ACT(act_singleflagging);
		ACT(act_safespots);
	}
	struct Action **res = (struct Action **) malloc(sizeof(struct Action *)*2);
	res[0] = (struct Action *) malloc(sizeof(struct Action));
	giveup(res[0]);
	res[1] = NULL;
	return res;
#undef ACT
}

static void actfree(struct Action **act) {
	int i = 0;
	while (act[i] != NULL) {
		free(act[i++]);
	}
	free(act);
}

void AI(struct Player *ply) {
	struct Player ai = {&act, &actfree};
	*ply = ai;
}
