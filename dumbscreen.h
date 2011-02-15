#ifndef DUMBSCREEN_H
#define DUMBSCREEN_H

#include <ncurses.h>
#include "Minefield.h"
#include "Screen.h"

class DumbScreen : public Screen<DumbScreen> {
	public:
		DumbScreen(Minefield *);
};

#endif
