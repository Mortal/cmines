#ifndef SCREEN_H
#define SCREEN_H

typedef void (*scrinit)(Minefield *f);
typedef void (*scrdeinit)(Minefield *);
typedef void (*scrupdatefield)(Minefield *, const char *field);
typedef void (*scrupdatetile)(Minefield *, int idx);
typedef void (*scrspeak)(Minefield *, const char *msg);

typedef struct _Screen {
	scrinit init;
	scrdeinit deinit;
	scrupdatefield updatefield;
	scrupdatetile updatetile;
	scrspeak speak;
	void *data;
} Screen;

#endif
