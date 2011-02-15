#ifndef SILENTSCREEN_H
#define SILENTSCREEN_H

#include <ncurses.h>
#include "Minefield.h"
#include "Screen.h"

class SilentScreen : public Screen<SilentScreen> {
	public:
		SilentScreen(Minefield *);
};

#endif
