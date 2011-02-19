#ifndef CMINES_H
#define CMINES_H

#include "types.h"
#include "Player.h"
#include "Screen.h"
#include "NCScreen.h"

#include <queue>
#include <vector>
#include <string>

char tilechar(Tile *tile);

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

	/* Tiles. */
	Tile *tiles;

	/* Number of entries in `tiles'. */
	int tilecount;
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

	/* Get the neighbouring indices of the tile at the given index and store them
	 * in `neighbours'. This output array should contain at least `maxneighbours'
	 * pointers to Coordinate sets, initially set to zero. */
	int *neighbourhood(int idx);
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

	/* Check if the game is over */
	void checkstate();

	enum FieldState state;

	/* Coordinate sets. Contains tilecount*dimcount Coordinates.
	 * Accessed via idxtocoords and coordstoidx. */
	Coordinate *coordinatesets;

	/* neighbourhood helper */
	void neighbourhood2(int root, int *neighbours, Dimension d, int times);
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

#endif
