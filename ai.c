#include <stdio.h>
#include <stdlib.h>
#include "ai.h"
#include "Player.h"

static void giveup(struct Action *act) {
	act->type = GIVEUP;
}

static struct Action **act() {
	struct Action **res = (struct Action **) malloc(sizeof(struct Action *)*2);
	int i;
	res[0] = (struct Action *) malloc(sizeof(struct Action));
	giveup(res[0]);
	res[1] = NULL;
	for (i = 0; i < tilecount; ++i) {
		struct Tile *tile = &tiles[i];
		if (!(tile->flags & (TILE_PRESSED|TILE_FLAGGED))) {
			struct Action act;
			act.type = PRESS;
			act.tileidx = i;
			*res[0] = act;
			break;
		}
	}
	return res;
}

static void actfree(struct Action **act) {
	struct Action **act_ = act;
	while (*act != NULL) {
		free(*act++);
	}
	free(act_);
}

void AI(struct Player *ply) {
	struct Player ai = {&act, &actfree};
	*ply = ai;
}
