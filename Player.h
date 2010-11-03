#ifndef PLAYER_H
#define PLAYER_H
#include "cmines.h"

enum ActionType {
	PRESS,
	FLAG,
	GIVEUP
};

struct Action {
	enum ActionType type;
	int tileidx;
};

typedef struct Action **(*plyactfun)(struct Minefield *);
typedef void (*plyfreefun)(struct Action **);

struct Player {
	plyactfun actfun;
	plyfreefun freefun;
};

#endif
