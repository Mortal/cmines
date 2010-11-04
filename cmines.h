#ifndef CMINES_H
#define CMINES_H

#define TRUE (1)
#define FALSE (0)

typedef unsigned int Coordinate;
typedef unsigned int Dimension;
typedef char bool;

#define TILE_MINE 0x1
#define TILE_PRESSED 0x2
#define TILE_FLAGGED 0x4
typedef struct {
	int flags;
	int neighbours;
} Tile;

char tilechar(Tile *tile);

enum FieldState {
	STATE_INIT,
	STATE_PLAY,
	STATE_WON,
	STATE_LOST
};

typedef struct {
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
	unsigned int tilecount;
	unsigned int maxneighbours;

	/* Some statistics. */
	unsigned int mines;
	unsigned int presseds;
	unsigned int flaggeds;

	/* whether mines was set automatically */
	bool automines;

	unsigned int outputwidth;
	unsigned int outputheight;

	/* Coordinate sets. Contains tilecount*dimcount Coordinates.
	 * Accessed via idxtocoords and coordstoidx. */
	Coordinate *coordinatesets;
} Minefield;

/* Get the column and row of the tile in the terminal/ncurses output. */
int outputcolumn(Minefield *, Coordinate *tile);
int outputrow(Minefield *, Coordinate *tile);

/* Convert tile index to coordinates. Returns a pointer into the huge
 * `coordinatesets' array. */
Coordinate *idxtocoords(Minefield *, int idx);

/* Convert coordinates to tile index. When passed a pointer into
 * `coordinatesets', calculates index by pointer arithmetic. */
unsigned int coordstoidx(Minefield *, Coordinate *c);

/* Get the neighbouring coordinate sets of the tile at the given index and
 * store them in `neighbours'. This output array should contain at least
 * `maxneighbours' pointers to Coordinate sets, initially set to zero. */
void neighbourhood(Minefield *, unsigned int idx, Coordinate **neighbours);

/* Various functions to initialise the global variables and create the game
 * field. */
void alloctiles(Minefield *);
void resettiles(Minefield *);
void calcmines(Minefield *);
void setmines(Minefield *);
void printfield(Minefield *);

#endif
