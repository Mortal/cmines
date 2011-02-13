#ifndef SCREEN_H
#define SCREEN_H

#define SCREENMARK_PRIMARY (1)
#define SCREENMARK_SECONDARY (2)

typedef void (*scrinit)(Minefield *f);
typedef void (*scrdeinit)(Minefield *);
typedef void (*scrupdatefield)(Minefield *, const char *field);
typedef void (*scrupdatetile)(Minefield *, int idx);
typedef void (*scrspeak)(Minefield *, const char *fmt, ...);
typedef void (*scrmark)(Minefield *, int idx, int mark);
typedef void (*scrresetmarks)(Minefield *);

typedef struct _Screen {
	scrinit init;
	scrdeinit deinit;
	scrupdatefield updatefield;
	scrupdatetile updatetile;
	scrspeak speak;
	scrmark mark;
	scrresetmarks resetmarks;
	void *data;
} Screen;

#endif
