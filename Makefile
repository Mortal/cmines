DEBUG=1

ifeq ($(DEBUG),)
	OPTS=-Wall -O3
else
	ifeq ($(PROFILE),)
		OPTS=-Wall -ggdb -DDEBUG
	else
		OPTS=-Wall -pg
	endif
endif

.cpp.o:
	g++ $(OPTS) -std=gnu++0x -I/usr/include/ncurses -c $<

OBJECTS = AI.o Minefield.o NCScreen.o NCPlayer.o DumbScreen.o SilentScreen.o

all: $(OBJECTS)
	g++ $(OPTS) -std=gnu++0x $(OBJECTS) -lncurses -o cmines

clean:
	rm -vf $(OBJECTS) cmines

AI.o: AI.cpp AI.h Minefield.h Player.h
NCPlayer.o: NCPlayer.cpp NCPlayer.h Minefield.h Player.h
NCScreen.o: NCScreen.cpp NCScreen.h Minefield.h Screen.h
DumbScreen.o: DumbScreen.cpp Minefield.h DumbScreen.h Screen.h
SilentScreen.o: SilentScreen.cpp Minefield.h SilentScreen.h Screen.h
Minefield.o: Minefield.cpp Minefield.h types.h NCScreen.h DumbScreen.h SilentScreen.h NCPlayer.h Player.h Screen.h AI.h

# DO NOT DELETE
