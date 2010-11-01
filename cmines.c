#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "cmines.h"

char tilechar(struct Tile *tile) {
	if (tile->flags & TILE_FLAGGED) return '/';
	if (tile->flags & TILE_MINE) return '@';
	if (tile->neighbours < 1) return ' ';
	if (tile->neighbours < 10) return '0'+tile->neighbours;
	if (tile->neighbours < 36) return 'A'+tile->neighbours-10;
	return 'Z';
}

int outputcolumn(Coordinate *tile) {
	int factor = 1;
	int sum = 0;
	Dimension d = dimcount-1;
	while (1) {
		sum += tile[d]*factor;
		if (d < 2) break;
		factor *= dimensions[d];
		++factor;
		d -= 2;
	}
	return sum;
}

int outputrow(Coordinate *tile) {
	int factor = 1;
	int sum = 0;
	Dimension d = dimcount-2;
	while (1) {
		sum += tile[d]*factor;
		if (d < 2) break;
		factor *= dimensions[d];
		++factor;
		d -= 2;
	}
	return sum;
}

void alloctiles() {
	if (tiles != 0) {
		free(tiles); tiles = 0;
	}
	if (coordinatesets != 0) {
		free(coordinatesets); coordinatesets = 0;
	}
	int i;
	tilecount = 1;
	maxneighbours = 1;
	for (i = 0; i < dimcount; ++i) {
		tilecount *= dimensions[i];
		maxneighbours *= 3;
	}
	tiles = (struct Tile *) malloc(tilecount*sizeof(struct Tile));
	struct Tile tile;
	for (i = 0; i < tilecount; ++i) {
		tiles[i] = tile;
	}
	coordinatesets = (Coordinate *) malloc(tilecount*dimcount*sizeof(Coordinate));
	for (i = 0; i < dimcount; ++i) {
		coordinatesets[i] = 0;
	}
	Coordinate *prev = idxtocoords(0);
	for (i = 1; i < tilecount; ++i) {
		Coordinate *cur = idxtocoords(i);
		int j, carry = 1;
		for (j = dimcount; j--;) {
			cur[j] = prev[j]+carry;
			carry = cur[j]/dimensions[j];
			cur[j] %= dimensions[j];
		}
		prev = cur;
	}
	outputheight = outputrow(idxtocoords(tilecount-1))+1;
	outputwidth = outputcolumn(idxtocoords(tilecount-1))+1;
}

Coordinate *idxtocoords(int idx) {
	return coordinatesets+idx*dimcount;
}

unsigned int coordstoidx(Coordinate *c) {
	if (c >= coordinatesets && c < coordinatesets+dimcount*tilecount) {
		return (c-coordinatesets)/dimcount;
	}
	unsigned int idx = 0;
	int i;
	for (i = 0; i < dimcount; ++i) {
		idx *= dimensions[i];
		idx += c[i];
	}
	return idx;
}

void neighbourhood(unsigned int idx, Coordinate **neighbours) {
	Coordinate *from = idxtocoords(idx);
	Coordinate basis[dimcount];
	int i;
	for (i = 0; i < dimcount; ++i)
		basis[i] = from[i];
	neighbourhood_(0, basis, 0, neighbours);
}

void neighbourhood_(Dimension dim, Coordinate *basis, bool includebasis, Coordinate **neighbours) {
	if (dim == dimcount) {
		if (includebasis) {
			while (*neighbours) ++neighbours;
			*neighbours = idxtocoords(coordstoidx(basis));
		}
	} else {
		if (basis[dim]) {
			--basis[dim];
			neighbourhood_(dim+1, basis, TRUE, neighbours);
			++basis[dim];
		}
		neighbourhood_(dim+1, basis, includebasis, neighbours);
		if (1+basis[dim] < dimensions[dim]) {
			++basis[dim];
			neighbourhood_(dim+1, basis, TRUE, neighbours);
			--basis[dim];
		}
	}
}

void resettiles() {
	int i;
	for (i = 0; i < tilecount; ++i) {
		tiles[i].flags = 0;
		tiles[i].neighbours = 0;
	}
	presseds = 0;
	flaggeds = 0;
}

void calcmines() {
	mines = tilecount/(effectivedimcount*effectivedimcount*effectivedimcount);
}

void setmines() {
	int i;
	for (i = 0; i < mines; ++i) {
		int j, idx = rand()%(tilecount-i);
		for (j = 0; j <= idx; ++j) {
			if (tiles[j].flags & TILE_MINE) ++idx;
		}
		assert(idx < tilecount);
		tiles[idx].flags |= TILE_MINE;
		Coordinate *neighbours[maxneighbours];
		for (j = 0; j < maxneighbours; ++j) {
			neighbours[j] = 0;
		}
		neighbourhood(idx, (Coordinate **) neighbours);
		for (j = 0; j < maxneighbours; ++j) {
			if (!neighbours[j]) break;
			++tiles[coordstoidx(neighbours[j])].neighbours;
		}
	}
}

void printfield() {
	char output[(outputwidth+1)*outputheight];
	{
		int row;
		for (row = 0; row < outputheight; ++row) {
			int column;
			for (column = 0; column < outputwidth; ++column) {
				output[row*(outputwidth+1)+column] = ' ';
			}
			output[row*(outputwidth+1)+column] = '\n';
		}
	}
	{
		int idx;
		for (idx = 0; idx < tilecount; ++idx) {
			int row = outputrow(idxtocoords(idx));
			int column = outputcolumn(idxtocoords(idx));
			output[row*(outputwidth+1)+column] = tilechar(tiles+idx);
		}
	}
	output[(outputwidth+1)*outputheight] = '\0';
	printf("%s", output);
}

int main(int argc, char *argv[]) {
	dimensions = malloc(sizeof(Coordinate)*2);
	dimensions[0] = 20;
	dimensions[1] = 40;
	dimcount = 2;
	effectivedimcount = 2;

	tiles = 0;
	coordinatesets = 0;

	srand(time(NULL) & 0xFFFFFFFF);
	alloctiles();
	resettiles();
	calcmines();
	setmines();
	printfield();
	return 0;
}
