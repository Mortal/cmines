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
		~NCPlayer();
		Action ** act();
		void free(Action **);

	private:
		Minefield *f;

		void setcursor();
		NCply *payload;
};

#endif
