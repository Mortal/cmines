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

typedef Action **(*plyactfun)(Minefield *);
typedef void (*plyfreefun)(Action **);

typedef struct {
	plyactfun actfun;
	plyfreefun freefun;
} Player;

#endif
