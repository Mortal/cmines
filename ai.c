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

#define ACT(method) static struct Action **method(struct Minefield *f, int idx)

ACT(act_dumb) {
	struct Tile *tile = &f->tiles[idx];
	if (!(tile->flags & (TILE_PRESSED|TILE_FLAGGED))) {
		struct Action **res = (struct Action **) malloc(sizeof(struct Action *)*2);

		struct Action act;
		act.type = PRESS;
		act.tileidx = idx;
		res[0] = (struct Action *) malloc(sizeof(struct Action));
		*res[0] = act;

		res[1] = NULL;

		return res;
	}
	return NULL;
}

#undef ACT

static struct Action **act(struct Minefield *f) {
#define ACT(method) {struct Action **ret = method(f, idx); if (ret != NULL) {allowcoordreset = 1; return ret;}}
	while (hasnexttile(f)) {
		int idx = nexttileidx(f);
		ACT(act_dumb);
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
