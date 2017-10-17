#ifndef CMINES_H
#define CMINES_H

#include "types.h"
#include "Player.h"
#include "Screen.h"
#include "NCScreen.h"

#include <queue>
#include <vector>
#include <string>

char tilechar(const Tile & tile);

struct neigh_t;

class Minefield {
public:
	Minefield();
	~Minefield();

	/* Field dimensions, most significant dimension first.
	 * I.e. {..., height, width}. */
	Coordinate *dimensions;

	/* Entry i is equal to the product of dimensions
	 * i, i+1, ..., dimensioncount-1 */
	Coordinate *dimensionproducts;

	/* Entry i is equal to the tile width of the ith dimension
	 * when rendering on a 2D screen. */
	std::vector<Coordinate> dimensiondrawproducts;

	/* Number of entries in `dimensions'. */
	Dimension dimcount;

	/* Number of entries in `dimensions' that aren't equal to 1.
	 * For mine calculation. */
	Dimension effectivedimcount;

	/* whether we should sleep occasionally */
	bool sleep;

	/* set dimensions and settings automatically and display usage options */
	bool usage;
	
	/* arguments used to run the program */
	std::vector<std::string> *args;

	const Tile & tile(int idx) const {
		return tiles[idx];
	}

	int getTilecount() const {
		return tilecount;
	}

	const Tile * tiles_begin() const {
		return tiles;
	}

	const Tile * tiles_end() const {
		return &tiles[tilecount];
	}

	int maxneighbours;

	int outputwidth;
	int outputheight;

	/* Get the column and row of the tile in the terminal/ncurses output. */
	int outputcolumn(Coordinate *tile);
	int outputrow(Coordinate *tile);

	/* Convert tile index to coordinates. Returns a pointer into the huge
	 * `coordinatesets' array. */
	Coordinate *idxtocoords(int idx);

	/* Convert coordinates to tile index. When passed a pointer into
	 * `coordinatesets', calculates index by pointer arithmetic.
	 * Note that this function is not actually used by anything */
#ifdef DEBUG
	int coordstoidx(Coordinate *c);
#endif

	int screentoidx(int row, int column);

	/* Get the neighbouring indices of the tile at the given index and store them
	 * in `neighbours'. This output array should contain at least `maxneighbours'
	 * pointers to Coordinate sets, initially set to zero. */
	neigh_t neighbourhood(int idx);
	void neighbourhood_free(int *neighbours);

	/* Various functions to initialise the global variables and create the game
	 * field. */
	void alloctiles();
	void resettiles();
	void calcmines();
	void setmines();

	/* Recalculate each tiles 'neighbours' count. Use after manipulating tile
	 * flags (which is considered cheating!) */
	void recalcneighbours();

	void printfield(char *);

	void redrawtile(int idx);
	void redrawfield();

	void mark(int idx, int mark);
	void resetmarks();

	void play();

	/* Some statistics. */
	int mines;
	int presseds;
	int flaggeds;

	/* seed passed to srand() before generating the minefield. */
	unsigned int seed;

	/* whether mines was set automatically */
	bool automines;

	/* expected result (arbitrary string) */
	const char *expect;

	bool ai; /* use AI? */

#define SCREEN_DUMB (0)
#define SCREEN_NCURSES (1)
#define SCREEN_SILENT (2)
	int screentype;

private:
	template <class ConcreteScreen>
	void playscreen(class Screen<ConcreteScreen> *scr);
	template <class ConcreteScreen, class ConcretePlayer>
	void playgame(class Screen<ConcreteScreen> *scr, class Player<ConcretePlayer> *ply);

	/* Tiles. */
	Tile *tiles;

	/* Number of entries in `tiles'. */
	int tilecount;

	/* Check if the game is over */
	void checkstate();

	enum FieldState state;

	/* Coordinate sets. Contains tilecount*dimcount Coordinates.
	 * Accessed via idxtocoords and coordstoidx. */
	Coordinate *coordinatesets;

	/* used in neighbourhood2() */
	int *neighbourhoodinc;

	/* neighbourhood helper */
	void neighbourhood2(int root, int *neighbours, Dimension d);
	/* allocated neighbourhood arrays that aren't in use */
	std::queue<int *> neighbourhoods;
	/* dealloc all in neighbourhoods */
	void neighbourhood_reallyfree();

	void press(int idx);
	bool simplepress(int idx);
	void ripplepress(int idx, std::queue<int> *);

	void pressrandom(bool blanksonly);
	void pressblanks();

	void flag(int idx);

	template <class ConcreteScreen> void flushredraws(Screen<ConcreteScreen> *);

	std::queue<int> redrawtiles;
	std::queue<Mark> marks;
	bool shouldredrawfield;
	bool shouldresetmarks;
};

struct neigh_t {
	inline neigh_t(int * src, Minefield & host) : src(src), host(host) {}
	inline int & operator[](int idx) {return src[idx];}
	inline ~neigh_t() {if (src) host.neighbourhood_free(src);}
	inline const int * operator+(int idx) const {return src + idx;}
	inline neigh_t(neigh_t && other) :
	src(other.src), host(other.host) {
		other.src = 0;
	}
	inline neigh_t & operator=(neigh_t && other) {
		if (this == &other) return *this;
		src = other.src;
		host = other.host;
		other.src = 0;
		return *this;
	}
private:
	int * src;
	Minefield & host;
	neigh_t(const neigh_t & other) : host(*((Minefield *) 0)) {}
};

#endif
