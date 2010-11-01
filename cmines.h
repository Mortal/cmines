#define TRUE (1)
#define FALSE (0)

typedef unsigned int Coordinate;
typedef unsigned int Dimension;
typedef char bool;

#define TILE_MINE 0x1
#define TILE_PRESSED 0x2
#define TILE_FLAGGED 0x4
struct Tile {
	int flags;
	int neighbours;
} Tile;

char tilechar(struct Tile *tile);

/* Field dimensions, most significant dimension first.
 * I.e. {..., height, width}. */
static Coordinate *dimensions;

/* Number of entries in `dimensions'. */
static Dimension dimcount;

/* Number of entries in `dimensions' that aren't equal to 1.
 * For mine calculation. */
static Dimension effectivedimcount;

/* Tiles. */
static struct Tile *tiles;

/* Number of entries in `tiles'. */
static unsigned int tilecount;
static unsigned int maxneighbours;

/* Some statistics. */
static unsigned int mines;
static unsigned int presseds;
static unsigned int flaggeds;

static unsigned int outputwidth;
static unsigned int outputheight;

/* Coordinate sets. Contains tilecount*dimcount Coordinates.
 * Accessed via idxtocoords and coordstoidx. */
static Coordinate *coordinatesets;

/* Get the column and row of the tile in the terminal/ncurses output. */
int outputcolumn(Coordinate *tile);
int outputrow(Coordinate *tile);

/* Convert tile index to coordinates. Returns a pointer into the huge
 * `coordinatesets' array. */
Coordinate *idxtocoords(int idx);

/* Convert coordinates to tile index. When passed a pointer into
 * `coordinatesets', calculates index by pointer arithmetic. */
unsigned int coordstoidx(Coordinate *c);

/* Get the neighbouring coordinate sets of the tile at the given index and
 * store them in `neighbours'. This output array should contain at least
 * `maxneighbours' pointers to Coordinate sets, initially set to zero. */
void neighbourhood(unsigned int idx, Coordinate **neighbours);
void neighbourhood_(Dimension dim, Coordinate *basis, bool includebasis,
		Coordinate **neighbours);

/* Various functions to initialise the global variables and create the game
 * field. */
void alloctiles();
void resettiles();
void calcmines();
void setmines();
void printfield();
