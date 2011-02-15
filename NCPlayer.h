#ifndef NCPLAYER_H
#define NCPLAYER_H

#include "Player.h"
#include "Minefield.h"
#include <ncurses.h>

typedef struct {
	int cursidx;
} NCply;

class NCPlayer : public Player<NCPlayer> {
	public:
		NCPlayer(Minefield *);
		void init(Minefield *);
		void deinit(Minefield *);
		Action ** act(Minefield *);
		void free(Action **);

	private:
		void setcursor(Minefield *f, WINDOW *scr);
		NCply *payload;
};

#endif
