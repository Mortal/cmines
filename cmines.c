#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "cmines.h"
#include "ai.h"

char tilechar(struct Tile *tile) {
	if (tile->flags & TILE_FLAGGED) return '/';
	if (!(tile->flags & TILE_PRESSED)) return '.';
	if (tile->flags & TILE_MINE) return '@';
	if (tile->neighbours < 1) return ' ';
	if (tile->neighbours < 10) return '0'+tile->neighbours;
	if (tile->neighbours < 36) return 'A'+tile->neighbours-10;
	return 'Z';
}

int outputcolumn(struct Minefield *f, Coordinate *tile) {
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

int outputrow(struct Minefield *f, Coordinate *tile) {
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

void alloctiles(struct Minefield *f) {
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
	f->tiles = (struct Tile *) malloc(f->tilecount*sizeof(struct Tile));
	struct Tile tile;
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

Coordinate *idxtocoords(struct Minefield *f, int idx) {
	return f->coordinatesets+idx*f->dimcount;
}

unsigned int coordstoidx(struct Minefield *f, Coordinate *c) {
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

void neighbourhood(struct Minefield *f, unsigned int idx, Coordinate **neighbours) {
	int i;
	for (i = 0; i < f->maxneighbours; ++i) {
		neighbours[i] = 0;
	}
	Coordinate *from = idxtocoords(f, idx);
	Coordinate basis[f->dimcount];
	for (i = 0; i < f->dimcount; ++i)
		basis[i] = from[i];
	neighbourhood_(f, 0, basis, 0, neighbours);
}

void neighbourhood_(struct Minefield *f, Dimension dim, Coordinate *basis, bool includebasis, Coordinate **neighbours) {
	if (dim == f->dimcount) {
		if (includebasis) {
			while (*neighbours) ++neighbours;
			*neighbours = idxtocoords(f, coordstoidx(f, basis));
		}
	} else {
		if (basis[dim]) {
			--basis[dim];
			neighbourhood_(f, dim+1, basis, TRUE, neighbours);
			++basis[dim];
		}
		neighbourhood_(f, dim+1, basis, includebasis, neighbours);
		if (1+basis[dim] < f->dimensions[dim]) {
			++basis[dim];
			neighbourhood_(f, dim+1, basis, TRUE, neighbours);
			--basis[dim];
		}
	}
}

void resettiles(struct Minefield *f) {
	int i;
	for (i = 0; i < f->tilecount; ++i) {
		f->tiles[i].flags = 0;
		f->tiles[i].neighbours = 0;
	}
	f->presseds = 0;
	f->flaggeds = 0;
}

void calcmines(struct Minefield *f) {
	f->mines = f->tilecount/(f->effectivedimcount*f->effectivedimcount*f->effectivedimcount);
}

void setmines(struct Minefield *f) {
	int i;
	for (i = 0; i < f->mines; ++i) {
		int j, idx = rand()%(f->tilecount-i);
		for (j = 0; j <= idx; ++j) {
			if (f->tiles[j].flags & TILE_MINE) ++idx;
		}
		assert(idx < f->tilecount);
		f->tiles[idx].flags |= TILE_MINE;
		Coordinate *neighbours[f->maxneighbours];
		neighbourhood(f, idx, (Coordinate **) neighbours);
		for (j = 0; j < f->maxneighbours; ++j) {
			if (!neighbours[j]) break;
			++f->tiles[coordstoidx(f, neighbours[j])].neighbours;
		}
	}
}

void press(struct Minefield *f, int idx) {
	struct Tile *tile = &f->tiles[idx];
	if (tile->flags & TILE_PRESSED) return;
	assert(!(tile->flags & TILE_PRESSED));
	if (tile->flags & TILE_FLAGGED) tile->flags &= ~TILE_FLAGGED;
	tile->flags |= TILE_PRESSED;
	assert(tile->flags & TILE_PRESSED);
	if (!tile->neighbours) {
		Coordinate *neighbours[f->maxneighbours];
		neighbourhood(f, idx, (Coordinate **) neighbours);
		int i;
		for (i = 0; i < f->maxneighbours && neighbours[i]; ++i) {
			press(f, coordstoidx(f, neighbours[i]));
		}
	}
}

void printfield(struct Minefield *f) {
	int w = f->outputwidth, h = f->outputheight;
	char output[(w+1)*h];
	{
		int row;
		for (row = 0; row < h; ++row) {
			int column;
			for (column = 0; column < w; ++column) {
				output[row*(w+1)+column] = ' ';
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
	printf("%s", output);
}

void pressrandom(struct Minefield *f, bool blanksonly) {
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

void pressblanks(struct Minefield *f) {
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
	struct Minefield f;
	f.dimcount = 0;
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
			++i;
		}
	}
	f.dimensions = malloc(sizeof(Coordinate)*f.dimcount);
	Dimension d = f.dimcount;
	for (i = 1; i < argc; ++i) {
		const char *arg = argv[i];
		if (isnumber(arg)) {
			int dim = strtol(arg, NULL, 0);
			f.dimensions[--d] = dim;
		} else if (!strcmp(arg, "--mines") || !strcmp(arg, "-m")) {
			++i;
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
	printfield(&f);
	struct Player ply;
	AI(&ply);
	while (1) {
		struct Action **act = (*ply.actfun)(&f);
		bool giveup = 0;
		int i = 0;
		while (act[i] != NULL) {
			struct Action *a = act[i++];
			if (a->type == GIVEUP) {
				printf("Giving up, aye\n");
				giveup = 1;
				break;
			}
			int tileidx = a->tileidx;
			if (a->type == PRESS) {
				press(&f, tileidx);
			}
			printfield(&f);
		}
		(*ply.freefun)(act);
		if (giveup) break;
	}
	return 0;
}
