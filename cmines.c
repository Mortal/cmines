#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <ncurses.h>
#include "cmines.h"
#include "ai.h"

char tilechar(Tile *tile) {
	if (tile->flags & TILE_FLAGGED) return '/';
	if (!(tile->flags & TILE_PRESSED)) return '.';
	if (tile->flags & TILE_MINE) return '@';
	if (tile->neighbours < 1) return ' ';
	if (tile->neighbours < 10) return '0'+tile->neighbours;
	if (tile->neighbours < 36) return 'A'+tile->neighbours-10;
	return 'Z';
}

int outputcolumn(Minefield *f, Coordinate *tile) {
	int factor = 1;
	int sum = 0;
	Dimension d = f->dimcount-1;
	while (1) {
		sum += tile[d]*factor;
		if (d < 2) break;
		factor *= f->dimensions[d];
		++factor;
		d -= 2;
	}
	return sum;
}

int outputrow(Minefield *f, Coordinate *tile) {
	int factor = 1;
	int sum = 0;
	Dimension d = f->dimcount-2;
	while (1) {
		sum += tile[d]*factor;
		if (d < 2) break;
		factor *= f->dimensions[d];
		++factor;
		d -= 2;
	}
	return sum;
}

void alloctiles(Minefield *f) {
	if (f->tiles != 0) {
		free(f->tiles); f->tiles = 0;
	}
	if (f->coordinatesets != 0) {
		free(f->coordinatesets); f->coordinatesets = 0;
	}
	int i;
	f->tilecount = 1;
	f->maxneighbours = 1;
	for (i = 0; i < f->dimcount; ++i) {
		f->tilecount *= f->dimensions[i];
		f->maxneighbours *= 3;
	}
	f->tiles = (Tile *) malloc(f->tilecount*sizeof(Tile));
	if (f->tiles == NULL) {
		printf("Not enough memory to allocate %d tiles!\n", f->tilecount);
		exit(2);
	}
	Tile tile;
	for (i = 0; i < f->tilecount; ++i) {
		f->tiles[i] = tile;
	}
	f->coordinatesets = (Coordinate *) malloc(f->tilecount*f->dimcount*sizeof(Coordinate));
	for (i = 0; i < f->dimcount; ++i) {
		f->coordinatesets[i] = 0;
	}
	Coordinate *prev = idxtocoords(f, 0);
	for (i = 1; i < f->tilecount; ++i) {
		Coordinate *cur = idxtocoords(f, i);
		int j, carry = 1;
		for (j = f->dimcount; j--;) {
			cur[j] = prev[j]+carry;
			carry = cur[j]/f->dimensions[j];
			cur[j] %= f->dimensions[j];
		}
		prev = cur;
	}
	f->outputheight = outputrow(f, idxtocoords(f, f->tilecount-1))+1;
	f->outputwidth = outputcolumn(f, idxtocoords(f, f->tilecount-1))+1;
}

Coordinate *idxtocoords(Minefield *f, int idx) {
	assert(idx >= 0 && idx <= f->tilecount);
	return f->coordinatesets+idx*f->dimcount;
}

unsigned int coordstoidx(Minefield *f, Coordinate *c) {
	if (c >= f->coordinatesets && c < f->coordinatesets+f->dimcount*f->tilecount) {
		return (c-f->coordinatesets)/f->dimcount;
	}
	unsigned int idx = 0;
	int i;
	for (i = 0; i < f->dimcount; ++i) {
		idx *= f->dimensions[i];
		idx += c[i];
	}
	return idx;
}

void neighbourhood(Minefield *f, unsigned int root, int *neighbours) {
	int idx = 0;
	Dimension dim;
	for (dim = 0; dim < f->dimcount; ++dim) {
		int tile = root;
		int i;
		int l = idx;
		for (i = 0; tile == root || i < l; tile = neighbours[(tile == root) ? i : ++i]) {
			Coordinate *basis = idxtocoords(f, tile);
			if (basis[dim]) {
				int i2 = tile-f->dimensionproducts[dim];
				neighbours[idx] = i2;
				idx++;
			}
			if (1+basis[dim] < f->dimensions[dim]) {
				int i2 = tile+f->dimensionproducts[dim];
				neighbours[idx] = i2;
				idx++;
			}
		}
	}
	neighbours[idx] = -1;
}

void resettiles(Minefield *f) {
	int i;
	for (i = 0; i < f->tilecount; ++i) {
		f->tiles[i].flags = 0;
		f->tiles[i].neighbours = 0;
	}
	f->presseds = 0;
	f->flaggeds = 0;
}

void calcmines(Minefield *f) {
	if (f->automines)
		f->mines = f->tilecount/(f->effectivedimcount*f->effectivedimcount*f->effectivedimcount);
}

void setmines(Minefield *f) {
	int i;
	for (i = 0; i < f->mines; ++i) {
		int j, idx = rand()%(f->tilecount-i);
		for (j = 0; j <= idx; ++j) {
			if (f->tiles[j].flags & TILE_MINE) ++idx;
		}
		assert(idx < f->tilecount);
		f->tiles[idx].flags |= TILE_MINE;
		int neighbours[f->maxneighbours];
		neighbourhood(f, idx, (int *) neighbours);
		for (j = 0; j < f->maxneighbours; ++j) {
			if (-1 == neighbours[j]) break;
			++f->tiles[neighbours[j]].neighbours;
		}
	}
}

void checkstate(Minefield *f) {
	if (f->mines+f->presseds >= f->tilecount) {
		f->state = STATE_WON;
	}
}

typedef struct {
	int *tilestart;
	int length;
	int first;
	int last;
	bool overflow;
} PressRipple;

void ripple_push(PressRipple *r, int idx) {
	if ((r->last+1) % r->length == r->first) {r->overflow = 1; return;}
	r->tilestart[r->last] = idx;
	//printf("Push %d=%d\n", r->last, idx);
	r->last = (r->last+1)%r->length;
}

int ripple_pop(PressRipple *r) {
	int val = r->tilestart[r->first];
	//printf("Pop %d=%d\n", r->first, val);
	r->first = (r->first+1)%r->length;
	return val;
}

bool simplepress(Minefield *f, int idx) {
	Tile *tile = &f->tiles[idx];
	if (tile->flags & TILE_PRESSED) return 0;
	assert(!(tile->flags & TILE_PRESSED));
	if (tile->flags & TILE_FLAGGED) tile->flags &= ~TILE_FLAGGED;
	tile->flags |= TILE_PRESSED;
	assert(tile->flags & TILE_PRESSED);
	if (tile->flags & TILE_MINE) {
		if (f->state == STATE_INIT) {
			printf("Pressed a mine during init!\n");
		}
		f->state = STATE_LOST;
		return 1;
	}
	++f->presseds;
	checkstate(f);
	return 1;
}

void ripplepress(Minefield *f, PressRipple *r) {
	int idx = ripple_pop(r);
	if (!simplepress(f, idx)) return;
	Tile *tile = &f->tiles[idx];
	if (tile->neighbours || r->overflow) return;
	int neighbours[f->maxneighbours];
	neighbourhood(f, idx, (int *) neighbours);
	int i;
	for (i = 0; !r->overflow && i < f->maxneighbours && neighbours[i] != -1; ++i) {
		ripple_push(r, neighbours[i]);
	}
}

static void handlepressoverflow(Minefield *f) {
	bool allowreset = 0;
	int idx;
	for (idx = 0; idx < f->tilecount || (allowreset && !(idx = 0) && !(allowreset = 0)); ++idx) {
		Tile *t = &f->tiles[idx];
		if (!(t->flags & TILE_PRESSED) || t->flags & TILE_FLAGGED || t->neighbours) continue;
		int neighbours[f->maxneighbours];
		neighbourhood(f, idx, (int *) neighbours);
		int i;
		for (i = 0; i < f->maxneighbours && neighbours[i] != -1; ++i) {
			if (simplepress(f, neighbours[i])) allowreset = 1;
		}
	}
}

void press(Minefield *f, int idx) {
	PressRipple r;
	int length = 1024;
	int positions[length];
	r.length = length;
	r.tilestart = positions;
	r.first = r.last = 0;
	ripple_push(&r, idx);
	while (r.first != r.last) {
		ripplepress(f, &r);
	}
	if (r.overflow) handlepressoverflow(f);
}

void flag(Minefield *f, int idx) {
	Tile *tile = &f->tiles[idx];
	if (tile->flags & TILE_FLAGGED) return;
	++f->flaggeds;
	tile->flags |= TILE_FLAGGED;
}

void printfield(Minefield *f) {
	int w = f->outputwidth, h = f->outputheight;
	char output[(w+1)*h];
	{
		int row;
		for (row = 0; row < h; ++row) {
			int column;
			for (column = 0; column < w; ++column) {
				output[row*(w+1)+column] = '+';
			}
			output[row*(w+1)+column] = '\n';
		}
	}
	{
		int idx;
		for (idx = 0; idx < f->tilecount; ++idx) {
			int row = outputrow(f, idxtocoords(f, idx));
			int column = outputcolumn(f, idxtocoords(f, idx));
			output[row*(w+1)+column] = tilechar(f->tiles+idx);
		}
	}
	output[(w+1)*h] = '\0';
	if (f->ncurses) {
		mvprintw(0, 0, "%s", output);
		refresh();
	} else {
		printf("%s", output);
	}
}

void pressrandom(Minefield *f, bool blanksonly) {
	int i;
	int eligible = 0;
	for (i = 0; i < f->tilecount; ++i) {
		if (!(f->tiles[i].flags & (TILE_MINE|TILE_FLAGGED|TILE_PRESSED)) && (!blanksonly || !f->tiles[i].neighbours)) {
			++eligible;
		}
	}
	if (!eligible) return;
	int idx = rand()%eligible;
	for (i = 0; i < f->tilecount; ++i) {
		if (!(f->tiles[i].flags & (TILE_MINE|TILE_FLAGGED|TILE_PRESSED)) && (!blanksonly || !f->tiles[i].neighbours)) {
			if (!idx--) {
				press(f, i);
				return;
			}
		}
	}
}

void pressblanks(Minefield *f) {
	int i;
	for (i = 0; i < f->tilecount; ++i) {
		if (!(f->tiles[i].flags & (TILE_MINE|TILE_FLAGGED|TILE_PRESSED)) && !f->tiles[i].neighbours) {
			press(f, i);
		}
	}
}

bool isnumber(const char *c) {
	if (*c == '\0') return 0;
	char *end;
	strtol(c, &end, 0);
	return *end == '\0';
}

int main(int argc, char *argv[]) {
	Minefield f;
	f.dimcount = 0;
	f.automines = 1;
	f.ncurses = 0;
	if (argc <= 2) {
		fprintf(stderr, "Usage: %s <width> <height> [<depth> [...]] [--mines <mines>]\n", argv[0]);
		exit(1);
	}
	int i;
	for (i = 1; i < argc; ++i) {
		const char *arg = argv[i];
		if (isnumber(arg)) {
			int dim = strtol(arg, NULL, 0);
			if (dim < 1) {
				fprintf(stderr, "Invalid argument %d\n", dim);
				exit(1);
			} else if (dim > 1) {
				++f.effectivedimcount;
			}
			++f.dimcount;
		} else if (!strcmp(arg, "--mines") || !strcmp(arg, "-m")) {
			// --mines takes a numerical argument, so skip that
			++i;
		}
	}
	f.dimensions = malloc(sizeof(Coordinate)*f.dimcount);
	f.dimensionproducts = malloc(sizeof(Coordinate)*f.dimcount);
	Dimension d = f.dimcount;
	for (i = 1; i < argc; ++i) {
		const char *arg = argv[i];
		if (isnumber(arg)) {
			int dim = strtol(arg, NULL, 0);
			int j;
			--d;
			if (i == 1) {
				for (j = 0; j < d; ++j) f.dimensionproducts[j] = dim;
				f.dimensionproducts[d] = 1;
			}
			else for (j = 0; j < d; ++j) f.dimensionproducts[j] *= dim;
			f.dimensions[d] = dim;
		} else if (!strcmp(arg, "--mines") || !strcmp(arg, "-m")) {
			++i;
			const char *arg2 = argv[i];
			if (isnumber(arg2)) {
				int mines = strtol(arg2, NULL, 0);
				if (mines < 1) {
					fprintf(stderr, "Invalid argument %d\n", mines);
					exit(1);
				}
				f.mines = mines;
				f.automines = 0;
			}
		} else if (!strcmp(arg, "--ncurses")) {
			f.ncurses = 1;
		}
	}

	f.tiles = 0;
	f.coordinatesets = 0;

	srand(time(NULL) & 0xFFFFFFFF);
	alloctiles(&f);
	resettiles(&f);
	calcmines(&f);
	setmines(&f);
	pressrandom(&f, 1);
	if (f.ncurses) initscr();
	printfield(&f);
	f.state = STATE_PLAY;
	Player ply;
	AI(&ply);
	time_t lastprint = 0;
	while (f.state == STATE_PLAY) {
		Action **act = (*ply.actfun)(&f);
		bool giveup = 0;
		int i = 0;
		while (act[i] != NULL) {
			Action *a = act[i++];
			if (a->type == GIVEUP) {
				giveup = 1;
				break;
			}
			int tileidx = a->tileidx;
			if (a->type == PRESS) {
				press(&f, tileidx);
			} else if (a->type == FLAG) {
				flag(&f, tileidx);
			}
		}
		time_t now = time(NULL);
		if (giveup || f.ncurses || f.state != STATE_PLAY || now != lastprint) {
			lastprint = now;
			printfield(&f);
		}
		(*ply.freefun)(act);
		if (giveup) break;
	}
	if (f.ncurses) {
		endwin();
		f.ncurses = 0;
		printfield(&f);
	}
	if (f.state == STATE_PLAY) {
		printf("You give up? Too bad!\n");
	} else if (f.state == STATE_LOST) {
		printf("Too bad!\n");
	} else if (f.state == STATE_WON) {
		printf("Congratulations!\n");
	}
	return 0;
}
