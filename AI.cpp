#include <stdio.h>
#include <stdlib.h>
#include "AI.h"
#include "Player.h"
#include "Screen.h"
#include <algorithm>

void AI::giveup(Action *act) {
	act->type = GIVEUP;
}

bool AI::hasnexttile() {
	return (this->allowcoordreset || this->nexttileidx_ < this->f->getTilecount());
}

int AI::nexttileidx() {
	if (this->nexttileidx_ < this->f->getTilecount()) {
		return this->nexttileidx_++;
	}
	this->allowcoordreset = 0;
	return this->nexttileidx_ = 0;
}

/* If the given tile has at least one of the given flags set, returns `result'
 * for the given tile. */
template<int flags, bool result>
class tilebyflag {
public:
	tilebyflag(const Minefield & field) : field(field) {}
	bool operator()(const int & tile) {
		return tile >= 0 && !result == !(field.tile(tile).flags & flags);
	}
private:
	const Minefield & field;
};

int AI::countunknown(const int *neighbours) const {
	// `unknown' tiles are tiles that are neither pressed nor flagged
	return std::count_if(neighbours, neighbours + this->f->maxneighbours,
		tilebyflag<TILE_PRESSED|TILE_FLAGGED, false>(*f));
}

int AI::countflags(const int *neighbours) const {
	// flagged tiles have the TILE_FLAGGED flag set
	return std::count_if(neighbours, neighbours + this->f->maxneighbours,
		tilebyflag<TILE_FLAGGED, true>(*f));
}

class filterunknown_cb {
public:
	filterunknown_cb(const Tile * tiles) : tiles(tiles) {}
	bool operator()(const int & tile) {
		return tile >= 0 && (tiles[tile].flags & (TILE_PRESSED|TILE_FLAGGED));
	}
private:
	const Tile * tiles;
};

void AI::filterunknown(int * neighbours) const {
	int * end = std::remove_if(neighbours, neighbours+f->maxneighbours,
			tilebyflag<TILE_PRESSED|TILE_FLAGGED, true>(*f));
	std::fill(end, neighbours+f->maxneighbours, -1);
}

template <typename IT, typename T>
class _sortedsetremove {
public:
	_sortedsetremove(IT begin, IT end) : begin(begin), end(end) {}
	bool operator()(const T & el) {
		while (begin != end && *begin < el)
			++begin;
		return el == *begin;
	}
private:
	IT begin;
	IT end;
};

template <typename T>
_sortedsetremove<T*, T> sortedsetremove(T* begin, T* end) {
	return _sortedsetremove<T*, T>(begin, end);
}

void AI::neighbourdifference(int *c, const int *set) {
	/* This is a more efficient way of saying
	 * int output[f->maxneighbours];
	 * int * end = std::set_difference(c, c+f->maxneighbours, set, set+f->maxneighbours, output);
	 * std::fill(end, &output[f->maxneighbours], -1);
	 * std::copy(&output[0], &output[f->maxneighbours], c);
	 * (using `output' since destination can't overlap source in set_difference) */
	int * end = std::remove_if(&c[0], &c[f->maxneighbours], sortedsetremove(&set[0], &set[f->maxneighbours]));
	std::fill(end, &c[f->maxneighbours], -1);
}

#define ACT(method) Action **AI::method(int idx)
#define GETTILE(var) const Tile & var = this->f->tile(idx)

ACT(act_singlecheck) {
	GETTILE(tile);
	if (!(tile.flags & TILE_PRESSED)) return NULL;
	if (!tile.neighbours) return NULL;
	int *neighbours = this->f->neighbourhood(idx);
	int neighbourunknown = countunknown((int *) neighbours);
	if (!neighbourunknown) return NULL;
	int neighbourflags = countflags((int *) neighbours);
	Action act;
	if (tile.neighbours == neighbourflags) {
		act.type = PRESS;
	} else if (tile.neighbours == neighbourunknown + neighbourflags) {
		act.type = FLAG;
	} else {
		this->f->neighbourhood_free(neighbours);
		return NULL;
	}

	Action **ret = new Action*[neighbourunknown+1];
	int retidx = 0;
	int i = 0;
	for (i = 0; i < this->f->maxneighbours; ++i) {
		int idx = neighbours[i];
		if (idx == -1) continue;
		const Tile & t = this->f->tile(idx);
		if (!(t.flags & (TILE_PRESSED|TILE_FLAGGED))) {
			act.tileidx = idx;
			Action *pact = ret[retidx++] = new Action;
			*pact = act;
		}
	}
	ret[retidx] = NULL;
	this->f->neighbourhood_free(neighbours);
	return ret;
}

bool AI::issubset(const int *superset, const int *subset, int length) {
	return std::includes(&superset[0], &superset[length], &subset[0], &subset[length]);
}


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
	int *an = this->f->neighbourhood(idx);

	// get a's bomb neighbour count minus already flagged bombs
	int anb = a.neighbours;
	{
		int i;
		for (i = 0; i < this->f->maxneighbours; ++i) {
			if (an[i] == -1) continue;
			if (this->f->tile(an[i]).flags & TILE_FLAGGED) {
				--anb;
			}
		}
	}

	// get a's unknown neighbourhood (unflagged, unpressed)
	int anu[this->f->maxneighbours];
	{
		int i;
		for (i = 0; i < this->f->maxneighbours; ++i) {
			anu[i] = an[i];
		}
	}
	filterunknown(anu);

	{
		int i;
		int *bn = NULL;
		for (i = 0; i < this->f->maxneighbours; ++i) {
			if (bn != NULL) {
				this->f->neighbourhood_free(bn);
				bn = NULL;
			}

			int bidx = an[i];
			if (bidx == -1) continue;
			const Tile & b = this->f->tile(bidx);

			if (!(b.flags & TILE_PRESSED)) continue;

			// get b's neighbourhood
			bn = this->f->neighbourhood(bidx);

			// get b's bomb neighbour count minus already flagged bombs
			int bnb = b.neighbours;
			{
				int i;
				for (i = 0; i < this->f->maxneighbours; ++i) {
					if (bn[i] == -1) continue;
					if (this->f->tile(bn[i]).flags & TILE_FLAGGED) {
						--bnb;
					}
				}
			}

			// get b's unknown neighbourhood (unflagged, unpressed)
			int bnu[this->f->maxneighbours];
			{
				int i;
				for (i = 0; i < this->f->maxneighbours; ++i) {
					bnu[i] = bn[i];
				}
			}
			filterunknown(bnu);

			neighbourdifference(bnu, anu);

			int count = 0;
			{
				int i;
				for (i = 0; i < this->f->maxneighbours; ++i) {
					if (bnu[i] != -1) ++count;
				}
			}
			if (!count) continue;

			Action act;
			if (count == bnb-anb) {
				act.type = FLAG;
			} else if (issubset(bnu, anu, this->f->maxneighbours) && bnb == anb) {
				act.type = PRESS;
			} else {
				continue;
			}
			Action **res = new Action*[count+1];
			int i, j = 0;
			for (i = 0; i < this->f->maxneighbours; ++i) {
				act.tileidx = bnu[i];
				if (act.tileidx == -1) continue;
				res[j] = new Action;
				*res[j] = act;
				j++;
			}
			res[j] = NULL;
			this->f->neighbourhood_free(an);
			this->f->neighbourhood_free(bn);
			return res;
		}
		if (bn != NULL) {
			this->f->neighbourhood_free(bn);
			bn = NULL;
		}
	}
	this->f->neighbourhood_free(an);
	return NULL;
}
#undef GETTILE
#undef ACT

Action **AI::act() {
#define ACT(method) {\
	Action **ret = this->method(idx);\
	if (ret != NULL) {\
		if (this->f->sleep) {\
			this->f->resetmarks();\
			this->f->mark(idx, 1);\
			int i;\
			for (i = 0; ret[i] != NULL; ++i) {\
				this->f->mark(ret[i]->tileidx, 2);\
			}\
		}\
		this->allowcoordreset = 1;\
		/*\
		char msg[256];\
		snprintf(msg, 255, "AI used %s\n", #method);\
		msg[255] = 0;\
		((Screen *) this->f->scr)->speak(this->f, msg);\
		printtile(this->f, idx);\
		*/\
		return ret;\
	}\
}
	// first, try the simple calculation on all tiles. act_singlecheck only calls
	// neighbourhood() once and some counting functions per call.
	while (this->hasnexttile()) {
		int idx = this->nexttileidx();
		ACT(act_singlecheck);
	}
	// once we've exhausted the playing field (run through from top to bottom
	// with no match), allowcoordreset is set to 0. reset it to 1 and try again
	// with the complex algorithm. act_dualcheck calls neighbourhood() once for
	// each tile and once again for each tile's pressed neighbours. this is a
	// slow operation! if we have a match, return it. next time, start over with
	// simple calculations.
	this->allowcoordreset = 1;
	while (this->hasnexttile()) {
		int idx = this->nexttileidx();
		ACT(act_dualcheck);
	}
	// we've exhausted the playing field twice now. we give up since the board is
	// ambiguous.
	Action **res = new Action*[2];
	res[0] = new Action;
	this->giveup(res[0]);
	res[1] = NULL;
	return res;
#undef ACT
}

void AI::free(Action **act) {
	int i = 0;
	while (act[i] != NULL) {
		delete act[i++];
	}
	delete act;
}

AI::AI(Minefield *f) {
	this->f = f;
	this->allowcoordreset = 0;
	this->nexttileidx_ = 0;
}
