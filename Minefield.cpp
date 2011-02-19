#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef DEBUG
#include <assert.h>
#endif
#include <time.h>
#include <unistd.h>
#include "Minefield.h"
#include "Screen.h"
#include "NCScreen.h"
#include "DumbScreen.h"
#include "SilentScreen.h"
#include "AI.h"
#include "NCPlayer.h"

char tilechar(Tile *tile) {
	if (tile->flags & TILE_FLAGGED) return '/';
	if (!(tile->flags & TILE_PRESSED)) return '.';
	if (tile->flags & TILE_MINE) return '@';
	if (tile->neighbours < 1) return ' ';
	if (tile->neighbours < 10) return '0'+tile->neighbours;
	if (tile->neighbours < 36) return 'A'+tile->neighbours-10;
	return 'Z';
}

int Minefield::outputcolumn(Coordinate *tile) {
	int factor = 1;
	int sum = 0;
	Dimension d = this->dimcount-1;
	while (1) {
		sum += tile[d]*factor;
		if (d < 2) break;
		factor *= this->dimensions[d];
		++factor;
		d -= 2;
	}
	return sum;
}

int Minefield::outputrow(Coordinate *tile) {
	int factor = 1;
	int sum = 0;
	Dimension d = this->dimcount-2;
	while (1) {
		sum += tile[d]*factor;
		if (d < 2) break;
		factor *= this->dimensions[d];
		++factor;
		d -= 2;
	}
	return sum;
}

void Minefield::alloctiles() {
	if (this->tiles != NULL) {
		delete this->tiles;
		this->tiles = NULL;
	}
	if (this->coordinatesets != NULL) {
		delete this->coordinatesets;
		this->coordinatesets = NULL;
	}
	int i;
	this->tilecount = 1;
	this->maxneighbours = 1;
	for (i = 0; i < this->dimcount; ++i) {
		this->tilecount *= this->dimensions[i];
		this->maxneighbours *= 3;
	}
	this->tiles = new Tile[this->tilecount];
	if (this->tiles == NULL) {
		printf("Not enough memory to allocate %d tiles!\n", this->tilecount);
		exit(2);
	}
	Tile tile;
	for (i = 0; i < this->tilecount; ++i) {
		this->tiles[i] = tile;
	}
	this->coordinatesets = new Coordinate[this->tilecount*this->dimcount];
	for (i = 0; i < this->dimcount; ++i) {
		this->coordinatesets[i] = 0;
	}
	Coordinate *prev = this->idxtocoords(0);
	for (i = 1; i < this->tilecount; ++i) {
		Coordinate *cur = this->idxtocoords(i);
		int j, carry = 1;
		for (j = this->dimcount; j--;) {
			cur[j] = prev[j]+carry;
			carry = cur[j]/this->dimensions[j];
			cur[j] %= this->dimensions[j];
		}
		prev = cur;
	}
	this->outputheight = this->outputrow(this->idxtocoords(this->tilecount-1))+1;
	this->outputwidth = this->outputcolumn(this->idxtocoords(this->tilecount-1))+1;
}

Coordinate *Minefield::idxtocoords(int idx) {
#ifdef DEBUG
	assert(idx >= 0 && idx <= this->tilecount);
#endif
	return this->coordinatesets+idx*this->dimcount;
}

#ifdef DEBUG
// not used by anything
int Minefield::coordstoidx(Coordinate *c) {
	if (c >= this->coordinatesets && c < this->coordinatesets+this->dimcount*this->tilecount) {
		return (c-this->coordinatesets)/this->dimcount;
	}
	int idx = 0;
	int i;
	for (i = 0; i < this->dimcount; ++i) {
		idx *= this->dimensions[i];
		idx += c[i];
	}
	return idx;
}
#endif

int *Minefield::neighbourhood(int root) {
	int *neighbours;
	if (!this->neighbourhoods.empty()) {
		neighbours = this->neighbourhoods.front();
		this->neighbourhoods.pop();
	} else {
		neighbours = new int[sizeof(int)*(this->maxneighbours)];
	}
	memset(neighbours, -1, sizeof(int)*(this->maxneighbours));
	neighbours[0] = root;
	this->neighbourhood2(root, neighbours, 0, 3);
	return neighbours;
}

void Minefield::neighbourhood_free(int *neighbours) {
	this->neighbourhoods.push(neighbours);
}

void Minefield::neighbourhood_reallyfree() {
	while (!this->neighbourhoods.empty()) {
		delete this->neighbourhoods.front();
		this->neighbourhoods.pop();
	}
}

void Minefield::neighbourhood2(int root, int *neighbours, Dimension d, int times) {
	if (d >= this->dimcount) return;
	int i, inc = this->maxneighbours/times;
	for (i = 0; i < this->maxneighbours; i += 3*inc) {
		int input = neighbours[i];
		if (input == -1) continue;
		int i2 = i + inc;
		int i3 = i2 + inc;
		neighbours[i2] = input;
		Coordinate *basis = this->idxtocoords(input);
		if (basis[d]) {
			neighbours[i] = input - this->dimensionproducts[d];
		} else {
			neighbours[i] = -1;
		}
		if (1+basis[d] < this->dimensions[d]) {
			neighbours[i3] = input + this->dimensionproducts[d];
		} else {
			neighbours[i3] = -1;
		}
		if (inc == 1 && neighbours[i2] == root) neighbours[i2] = -1;
	}
	this->neighbourhood2(root, neighbours, d+1, times*3);
}

void Minefield::resettiles() {
	int i;
	for (i = 0; i < this->tilecount; ++i) {
		this->tiles[i].flags = 0;
		this->tiles[i].neighbours = 0;
	}
	this->presseds = 0;
	this->flaggeds = 0;
}

void Minefield::calcmines() {
	if (this->automines) {
		this->mines = this->tilecount/(this->effectivedimcount*this->effectivedimcount*this->effectivedimcount);
		if (this->ai) {
			switch (this->effectivedimcount) {
				case 1:
				case 2:
					break;
				case 3:
					this->mines = this->mines*5/2;
					break;
				default:
					this->mines = this->mines*7/2;
			}
		}
	}
}

void Minefield::setmines() {
	int i;
	for (i = 0; i < this->mines; ++i) {
		int j, idx = rand()%(this->tilecount-i);
		for (j = 0; j <= idx; ++j) {
			if (this->tiles[j].flags & TILE_MINE) ++idx;
		}
#ifdef DEBUG
		assert(idx < this->tilecount);
#endif
		this->tiles[idx].flags |= TILE_MINE;
		int *neighbours = this->neighbourhood(idx);
		for (j = 0; j < this->maxneighbours; ++j) {
			if (-1 == neighbours[j]) continue;
			++this->tiles[neighbours[j]].neighbours;
		}
		this->neighbourhood_free(neighbours);
	}
}

void Minefield::recalcneighbours() {
	int i;
	for (i = 0; i < this->tilecount; ++i) {
		this->tiles[i].neighbours = 0;
	}
	for (i = 0; i < this->tilecount; ++i) {
		if (!(this->tiles[i].flags & TILE_MINE)) continue;
		int *neighbours = this->neighbourhood(i);
		int j;
		for (j = 0; j < this->maxneighbours; ++j) {
			if (-1 == neighbours[j]) continue;
			++this->tiles[neighbours[j]].neighbours;
		}
		this->neighbourhood_free(neighbours);
	}
}

void Minefield::checkstate() {
	if (this->mines+this->presseds >= this->tilecount) {
		this->state = STATE_WON;
	}
}

void Minefield::redrawtile(int idx) {
	this->redrawtiles.push(idx);
}

void Minefield::redrawfield() {
	this->shouldredrawfield = true;
}

void Minefield::mark(int idx, int mark) {
	Mark m;
	m.idx = idx;
	m.mark = mark;
	this->marks.push(m);
}

void Minefield::resetmarks() {
	this->shouldresetmarks = true;
}

bool Minefield::simplepress(int idx) {
#ifdef DEBUG
	assert(idx >= 0 && idx <= this->tilecount);
#endif
	Tile *tile = &this->tiles[idx];
	if (tile->flags & TILE_PRESSED) return 0;
#ifdef DEBUG
	assert(!(tile->flags & TILE_PRESSED));
#endif
	if (tile->flags & TILE_FLAGGED) tile->flags &= ~TILE_FLAGGED;
	tile->flags |= TILE_PRESSED;
#ifdef DEBUG
	assert(tile->flags & TILE_PRESSED);
#endif
	if (tile->flags & TILE_MINE) {
		if (this->state == STATE_INIT) {
			printf("Pressed a mine during init!\n");
		}
		this->state = STATE_LOST;
		return 1;
	}
	++this->presseds;
	this->redrawtile(idx);
	this->checkstate();
	return 1;
}

void Minefield::ripplepress(int idx, std::queue<int> *queue) {
	if (!this->simplepress(idx)) return;
	Tile *tile = &this->tiles[idx];
	if (tile->neighbours) return;
	int *neighbours = this->neighbourhood(idx);
	int i;
	for (i = 0; i < this->maxneighbours; ++i) {
		if (neighbours[i] == -1) continue;
		queue->push(neighbours[i]);
	}
	this->neighbourhood_free(neighbours);
}

void Minefield::press(int idx) {
#ifdef DEBUG
	assert(idx >= 0 && idx <= this->tilecount);
#endif
	std::queue<int> tilestopress;
	tilestopress.push(idx);
	while (!tilestopress.empty()) {
		int tile = tilestopress.front();
		tilestopress.pop();
		this->ripplepress(tile, &tilestopress);
	}
}

void Minefield::flag(int idx) {
	Tile *tile = &this->tiles[idx];
	if (tile->flags & TILE_FLAGGED) return;
	++this->flaggeds;
	tile->flags |= TILE_FLAGGED;
	this->redrawtile(idx);
}

void Minefield::printfield(char *output) {
	int w = this->outputwidth, h = this->outputheight;
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
		for (idx = 0; idx < this->tilecount; ++idx) {
			int row = this->outputrow(this->idxtocoords(idx));
			int column = this->outputcolumn(this->idxtocoords(idx));
			output[row*(w+1)+column] = tilechar(this->tiles+idx);
		}
	}
	output[(w+1)*h] = '\0';
}

void Minefield::pressrandom(bool blanksonly) {
	int i;
	int eligible = 0;
	for (i = 0; i < this->tilecount; ++i) {
		if (!(this->tiles[i].flags & (TILE_MINE|TILE_FLAGGED|TILE_PRESSED)) && (!blanksonly || !this->tiles[i].neighbours)) {
			++eligible;
		}
	}
	if (!eligible) return;
	int idx = rand()%eligible;
	for (i = 0; i < this->tilecount; ++i) {
		if (!(this->tiles[i].flags & (TILE_MINE|TILE_FLAGGED|TILE_PRESSED)) && (!blanksonly || !this->tiles[i].neighbours)) {
			if (!idx--) {
				this->press(i);
				return;
			}
		}
	}
}

void Minefield::pressblanks() {
	int i;
	for (i = 0; i < this->tilecount; ++i) {
		if (!(this->tiles[i].flags & (TILE_MINE|TILE_FLAGGED|TILE_PRESSED)) && !this->tiles[i].neighbours) {
			this->press(i);
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
	if (argc <= 2) {
		fprintf(stderr, "Usage: %s <width> <height> [<depth> [...]] [--mines <mines>]\n", argv[0]);
		return 1;
	}
	Minefield *f = new Minefield;
	bool hasseed = 0;
	int i;
	for (i = 1; i < argc; ++i) {
		const char *arg = argv[i];
		if (isnumber(arg)) {
			int dim = strtol(arg, NULL, 0);
			if (dim < 1) {
				fprintf(stderr, "Invalid argument %d\n", dim);
				exit(1);
			} else if (dim > 1) {
				++f->effectivedimcount;
			}
			++f->dimcount;
		} else if (!strcmp(arg, "--mines") || !strcmp(arg, "-m") || !strcmp(arg, "--seed") || !strcmp(arg, "--expect")) {
			// takes an argument, so skip that
			++i;
		}
	}
	f->dimensions = new Coordinate[f->dimcount];
	f->dimensionproducts = new Coordinate[f->dimcount];
	Dimension d = f->dimcount;
	for (i = 1; i < argc; ++i) {
		const char *arg = argv[i];
		if (isnumber(arg)) {
			int dim = strtol(arg, NULL, 0);
			int j;
			--d;
			if (i == 1) {
				for (j = 0; j < d; ++j) f->dimensionproducts[j] = dim;
				f->dimensionproducts[d] = 1;
			}
			else for (j = 0; j < d; ++j) f->dimensionproducts[j] *= dim;
			f->dimensions[d] = dim;
		} else if (!strcmp(arg, "--mines") || !strcmp(arg, "-m")) {
			++i;
			const char *arg2 = argv[i];
			if (isnumber(arg2)) {
				int mines = strtol(arg2, NULL, 0);
				if (mines < 1) {
					fprintf(stderr, "Invalid argument %d\n", mines);
					exit(1);
				}
				f->mines = mines;
				f->automines = 0;
			}
		} else if (!strcmp(arg, "--seed")) {
			++i;
			const char *arg2 = argv[i];
			if (isnumber(arg2)) {
				int seed = strtol(arg2, NULL, 0);
				f->seed = seed;
				hasseed = 1;
			}
		} else if (!strcmp(arg, "--expect")) {
			++i;
			f->expect = argv[i];
		} else if (!strcmp(arg, "--ncurses")) {
			f->screentype = SCREEN_NCURSES;
		} else if (!strcmp(arg, "--silent")) {
			f->screentype = SCREEN_SILENT;
		} else if (!strcmp(arg, "--manual")) {
			f->ai = 0;
		} else if (!strcmp(arg, "-s") || !strcmp(arg, "--sleep")) {
			f->sleep = 1;
		}
	}

	if (!hasseed) {
		srand(time(NULL) & 0xFFFFFFFF);
		f->seed = rand();
	}
	f->play();
	delete f;
	return 0;
}

Minefield::Minefield():
	dimensions(NULL),
	dimensionproducts(NULL),
	dimcount(0),
	effectivedimcount(0),
	sleep(0),
	tiles(NULL),
	tilecount(0),
	maxneighbours(0),
	outputwidth(0),
	outputheight(0),
	mines(0),
	presseds(0),
	flaggeds(0),
	seed(0),
	automines(1),
	expect(NULL),
	ai(1),
	screentype(SCREEN_DUMB),
	state(STATE_INIT),
	coordinatesets(NULL),
	shouldredrawfield(0),
	shouldresetmarks(0)
{
}

Minefield::~Minefield() {
#define MAYBEFREE(prop) if (this->prop != NULL) {\
		delete this->prop;\
		this->prop = NULL;\
	}
	MAYBEFREE(coordinatesets);
	MAYBEFREE(tiles);
	MAYBEFREE(dimensions);
	MAYBEFREE(dimensionproducts);
	this->neighbourhood_reallyfree();
}

void Minefield::play() {
	srand(this->seed);
	this->alloctiles();
	this->resettiles();
	this->calcmines();
	this->setmines();
	this->pressrandom(1);

#define PLAYSCREEN(SCREEN) {SCREEN *scr = new SCREEN(this);\
	this->playscreen(scr);\
	delete scr;\
}
	switch (this->screentype) {
		case SCREEN_NCURSES:
			PLAYSCREEN(NCScreen);
			break;
		case SCREEN_SILENT:
			PLAYSCREEN(SilentScreen);
			break;
		default:
			PLAYSCREEN(DumbScreen);
			break;
	}
}

template <class ConcreteScreen>
void Minefield::playscreen(Screen<ConcreteScreen> *scr) {
	scr->init();
	scr->speak("Seed: %u\n", this->seed);
	this->redrawfield();
	this->state = STATE_PLAY;

	if (this->ai) {
		Player<AI> *ply = new AI(this);
		this->playgame(scr, ply);
		delete ply;
	} else {
		Player<NCPlayer> *ply = new NCPlayer(this);
		this->playgame(scr, ply);
		delete ply;
	}

	const char *msg = "No message";
	const char *expect = "huh";
	if (this->state == STATE_PLAY) {
		msg = "You give up? Too bad!\n";
		expect = "giveup";
	} else if (this->state == STATE_LOST) {
		msg = "Too bad!\n";
		expect = "loss";
	} else if (this->state == STATE_WON) {
		msg = "Congratulations!\n";
		expect = "win";
	}
	scr->speak(msg);
	if (this->sleep) usleep(800000);
	scr->deinit();
	if (this->expect != NULL) {
		exit(strcmp(expect, this->expect) ? 1 : 0);
	}
	printf("To reproduce, run with ");
	{
		Dimension d;
		for (d = this->dimcount; d && d--;) {
			printf(" %d", this->dimensions[d]);
		}
	}
	printf(" --mines %d --seed %d --expect %s\n", this->mines, this->seed, expect);
}

template <class ConcreteScreen>
void Minefield::flushredraws(Screen<ConcreteScreen> *scr) {
	if (this->shouldredrawfield) {
		char output[(this->outputwidth+1) * this->outputheight];
		this->printfield(output);
		scr->updatefield(output);
		this->shouldredrawfield = false;
	}
	while (!this->redrawtiles.empty()) {
		scr->updatetile(this->redrawtiles.front());
		this->redrawtiles.pop();
	}
	if (this->shouldresetmarks) {
		scr->resetmarks();
		this->shouldresetmarks = false;
	}
	while (!this->marks.empty()) {
		Mark m = this->marks.front();
		scr->mark(m);
		this->marks.pop();
	}
}

template <class ConcreteScreen, class ConcretePlayer>
void Minefield::playgame(Screen<ConcreteScreen> *scr, Player<ConcretePlayer> *ply) {
	while (this->state == STATE_PLAY) {
		this->flushredraws(scr);
		Action **act = ply->act();
		bool giveup = 0;
		int i = 0;
		while (act[i] != NULL) {
			Action *a = act[i++];
			if (a->type == GIVEUP) {
				giveup = 1;
				break;
			}
			if (a->type == NOOP) {
				continue;
			}
			int tileidx = a->tileidx;
			if (a->type == PRESS) {
				this->press(tileidx);
			} else if (a->type == FLAG) {
				this->flag(tileidx);
			}
		}
		if (giveup || this->state != STATE_PLAY) {
			this->redrawfield();
		}
		this->flushredraws(scr);
		ply->free(act);
		if (giveup) break;
		if (this->sleep) {
			usleep(150000);
		}
	}
}
