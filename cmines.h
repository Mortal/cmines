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

static unsigned int mines;
static unsigned int presseds;
static unsigned int flaggeds;

static unsigned int outputwidth;
static unsigned int outputheight;

/* Coordinate sets. tilecount*dimcount Coordinates.
 * Accessed via idxtocoords and coordstoidx. */
static Coordinate *coordinatesets;

int outputcolumn(Coordinate *tile);
int outputrow(Coordinate *tile);
void alloctiles();
Coordinate *idxtocoords(int idx);
unsigned int coordstoidx(Coordinate *c);
void neighbourhood(unsigned int idx, Coordinate **neighbours);
void neighbourhood_(Dimension dim, Coordinate *basis, bool includebasis, Coordinate **neighbours);
void resettiles();
void calcmines();
void setmines();
void printfield();
