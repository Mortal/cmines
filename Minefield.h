#ifndef CMINES_H
#define CMINES_H

#include "types.h"
#include "Player.h"
#include "Screen.h"
#include "NCScreen.h"

#include <queue>

char tilechar(Tile *tile);

typedef struct {
	int *tilestart;
	int length;
	int first;
	int last;
	bool overflow;
} PressRipple;

class Minefield {
public:
	/* Field dimensions, most significant dimension first.
	 * I.e. {..., height, width}. */
	Coordinate *dimensions;

	/* Entry i is equal to the product of dimensions
	 * i, i+1, ..., dimensioncount-1 */
	Coordinate *dimensionproducts;

	enum FieldState state;

	/* Number of entries in `dimensions'. */
	Dimension dimcount;

	/* Number of entries in `dimensions' that aren't equal to 1.
	 * For mine calculation. */
	Dimension effectivedimcount;

	/* Tiles. */
	Tile *tiles;

	/* Number of entries in `tiles'. */
	int tilecount;
	int maxneighbours;

	/* Some statistics. */
	int mines;
	int presseds;
	int flaggeds;

	/* whether mines was set automatically */
	bool automines;

	int outputwidth;
	int outputheight;

	/* Coordinate sets. Contains tilecount*dimcount Coordinates.
	 * Accessed via idxtocoords and coordstoidx. */
	Coordinate *coordinatesets;

	/* whether we should sleep occasionally */
	bool sleep;

	/* seed passed to srand() before generating the minefield. */
	unsigned int seed;

	/* expected result (arbitrary string) */
	const char *expect;

	/* Get the column and row of the tile in the terminal/ncurses output. */
	int outputcolumn(Coordinate *tile);
	int outputrow(Coordinate *tile);

	/* Convert tile index to coordinates. Returns a pointer into the huge
	 * `coordinatesets' array. */
	Coordinate *idxtocoords(int idx);

	/* Convert coordinates to tile index. When passed a pointer into
	 * `coordinatesets', calculates index by pointer arithmetic. */
	int coordstoidx(Coordinate *c);

	/* Get the neighbouring indices of the tile at the given index and store them
	 * in `neighbours'. This output array should contain at least `maxneighbours'
	 * pointers to Coordinate sets, initially set to zero. */
	void neighbourhood(int idx, int *neighbours);

	/* Various functions to initialise the global variables and create the game
	 * field. */
	void alloctiles();
	void resettiles();
	void calcmines();
	void setmines();
	void recalcneighbours();
	void printfield(char *);

	void redrawtile(int idx);
	void redrawfield();

	void mark(int idx, int mark);
	void resetmarks();

	int main(int argc, char *argv[]);

private:
	template <class ConcreteScreen>
	void playscreen(class Screen<ConcreteScreen> *scr);
	template <class ConcreteScreen, class ConcretePlayer>
	void playgame(class Screen<ConcreteScreen> *scr, class Player<ConcretePlayer> *ply);

	/* Check if the game is over */
	void checkstate();

	/* neighbourhood helper */
	void neighbourhood2(int root, int *neighbours, Dimension d, int times);

	void press(int idx);
	bool simplepress(int idx);
	void ripplepress(PressRipple *);
	void handlepressoverflow();

	void pressrandom(bool blanksonly);
	void pressblanks();

	void flag(int idx);

	bool ai; /* use AI? */

	std::queue<int> *redrawtiles;
	std::queue<Mark *> *marks;
	bool shouldredrawfield;
	bool shouldresetmarks;
};

#endif
