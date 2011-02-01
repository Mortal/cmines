#ifndef TYPES_H
#define TYPES_H

#ifndef TRUE
	#define TRUE (1)
#endif
#ifndef FALSE
	#define FALSE (0)
#endif

typedef unsigned int Coordinate;
typedef unsigned int Dimension;
#ifndef bool
	typedef char bool;
#endif

#define TILE_MINE 0x1
#define TILE_PRESSED 0x2
#define TILE_FLAGGED 0x4
typedef struct {
	int flags;
	int neighbours;
} Tile;

enum FieldState {
	STATE_INIT,
	STATE_PLAY,
	STATE_WON,
	STATE_LOST
};


#endif
