#ifndef PLAYER_H
#define PLAYER_H
#include "cmines.h"

enum ActionType {
	PRESS,
	FLAG,
	GIVEUP
};

typedef struct {
	enum ActionType type;
	int tileidx;
} Action;

struct _Player;

typedef void (*plyinitfun)(struct _Player *, Minefield *);
typedef void (*plydeinitfun)(struct _Player *, Minefield *);
typedef Action **(*plyactfun)(struct _Player *, Minefield *);
typedef void (*plyfreefun)(struct _Player *, Action **);

typedef struct _Player {
	plyinitfun initfun;
	plydeinitfun deinitfun;
	plyactfun actfun;
	plyfreefun freefun;
	void *payload;
} Player;

#endif
