#include <stdio.h>
#include <stdlib.h>
#include "ai.h"
#include "Player.h"

bool allowcoordreset = 0;
int nexttileidx_ = 0;

/* what follows is a binary search tree implementation based on Cormen et al,
 * Introduction to algorithms 2nd ed */

typedef struct _neighbournode {
	int idx;
	struct _neighbournode *left;
	struct _neighbournode *right;
	struct _neighbournode *parent;
} neighbournode;

int amount = 0;

static void neighbourtreeinsert(neighbournode *x, int idx) {
	//printf("(%d) Insert %d in tree rooted by %d\n", amount, idx, x->idx);
	neighbournode *y = NULL;
	while (x != NULL) {
		y = x;
		if (idx < x->idx) {
			x = x->left;
		} else {
			x = x->right;
		}
	}
	if (y == NULL) {
		x->idx = idx;
		x->left = x->right = x->parent = NULL;
	} else {
		neighbournode *insert = (neighbournode *) malloc(sizeof(neighbournode));
		++amount;
		insert->idx = idx;
		insert->parent = y;
		insert->left = insert->right = NULL;
		if (idx < y->idx) {
			y->left = insert;
		} else {
			y->right = insert;
		}
	}
}

/*
static neighbournode *neighbourtreeminimum(neighbournode *node) {
	while (1) {
		neighbournode *left = node->left;
		if (left == NULL) break;
		node = left;
	}
	return node;
}

static neighbournode *neighbourtreesuccessor(neighbournode *node) {
	if (node->right != NULL) return neighbourtreeminimum(node->right);
	neighbournode *y = node->parent;
	while (y != NULL && x == y->right) {
		node = y;
		y = y->parent;
	}
	return y;
}

static void neighbourtreedelete(neighbournode *node) {
	neighbournode *x;
	neighbournode *y;
	if (node->left == NULL || node->right == NULL) {
		y = node;
	} else {
		y = neighbourtreesuccessor(node);
	}
	if (y->left != NULL) {
		x = y->left;
	} else {
		x = y->right;
	}
	if (x != NULL) {
		x->parent = y->parent;
	}
	if (y->parent == NULL) {
		y->idx = x->idx;
}
*/

static neighbournode *makeneighbourtree(int *neighbours) {
	neighbournode *root = (neighbournode *) malloc(sizeof(neighbournode));
	root->left = root->right = root->parent = NULL;
	++amount;
	int i; int insert;
	for (i = 0, insert = neighbours[i]; insert != -1; ++i, insert = neighbours[i]) {
		if (i == 0) {
			root->idx = insert;
		} else {
			neighbourtreeinsert(root, insert);
		}
	}
	return root;
}

static void neighbourtree_free(neighbournode *root) {
	if (root == NULL) return;
	neighbourtree_free(root->left);
	neighbourtree_free(root->right);
	//printf("(%d) Free %d\n", --amount, root->idx);
	free(root);
}

static neighbournode *neighbourtree_find(neighbournode *root, int idx) {
	while (root != NULL) {
		int found = root->idx;
		if (found == idx) {
			return root;
		} else if (idx < found) {
			root = root->left;
		} else {
			root = root->right;
		}
	}
	return NULL;
}

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
/*
CB(neighbourunpressed_cb) {
	return !(tile->flags & TILE_PRESSED);
}
CB(neighbournoflags_cb) {
	return !(tile->flags & TILE_FLAGGED);
}
CB(neighbourneighbour_cb) {
	return (tile->flags & TILE_PRESSED) && (tile->neighbours > 0);
}
*/
CB(neighbourunknown_cb) {
	return !(tile->flags & (TILE_PRESSED|TILE_FLAGGED));
}
CB(neighbourflags_cb) {
	return !!(tile->flags & TILE_FLAGGED);
}
CB(neighbourdifference_cb) {
	neighbournode *set = (neighbournode *) payload;
	if (NULL == neighbourtree_find(set, idx)) return 1;
	return 0;
}
#undef CB

#define ACT(method) static Action **method(Minefield *f, int idx)
#define GETTILE(tile) Tile *tile = &f->tiles[idx]

/*
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
*/

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

static bool issubset(neighbournode *superset, int *subset) {
	while (*subset != -1) {
		if (NULL == neighbourtree_find(superset, *subset)) return 0;
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

	neighbournode *anut = makeneighbourtree(anu);

	Action **res = NULL;
	{
		int i = 0;
		while (an[i] != -1) {
			int bidx = an[i++];
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
				int i = 0;
				while (bn[i] != -1) {
					if (f->tiles[bn[i++]].flags & TILE_FLAGGED) {
						if (debug) printtile(f, bn[i-1]);
						--bnb;
					}
				}
			}

			if (debug) printf("A has %d/%d neighbours and B has %d/%d neighbours\n", anb, a->neighbours, bnb, b->neighbours);

			// get b's unknown neighbourhood (unflagged, unpressed)
			int bnu[f->maxneighbours];
			{ int *a = bn; int *b = bnu; while ((*b++ = *a++) != -1); }
			neighbourfilter(f, bnu, &neighbourunknown_cb, NULL);

			neighbourfilter(f, bnu, &neighbourdifference_cb, anut);

			neighbournode *bnut = makeneighbourtree(bnu);

			int count = 0; while (bnu[count] != -1) ++count;
			if (!count) continue;

			Action act;
			if (count == bnb-anb) {
				act.type = FLAG;
			} else if (issubset(bnut, anu) && bnb == anb) {
				act.type = PRESS;
			} else {
				neighbourtree_free(bnut);
				continue;
			}
			neighbourtree_free(bnut);
			res = (Action **) malloc(sizeof(Action *)*(count+1));
			int i = 0;
			while (bnu[i] != -1) {
				act.tileidx = bnu[i];
				res[i] = (Action *) malloc(sizeof(Action));
				*res[i] = act;
				i++;
			}
			res[i] = NULL;
			break;
		}
	}
	neighbourtree_free(anut);
	return res;
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
