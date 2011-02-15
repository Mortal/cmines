DEBUG=1

ifeq ($(DEBUG),)
	OPTS=-Wall -O3
else
	OPTS=-Wall -ggdb -DDEBUG
endif

.cpp.o:
	g++ $(OPTS) -c $<

OBJECTS = AI.o Minefield.o ncscreen.o NCPlayer.o

all: $(OBJECTS)
	g++ $(OPTS) -lncurses $(OBJECTS) -o cmines

clean:
	rm -vf $(OBJECTS) cmines

AI.o: AI.cpp AI.h Minefield.h Player.h
NCPlayer.o: NCPlayer.cpp NCPlayer.h Minefield.h Player.h
ncscreen.o: ncscreen.cpp ncscreen.h Minefield.h Screen.h
dumbscreen.o: dumbscreen.cpp Minefield.h dumbscreen.h Screen.h
silentscreen.o: silentscreen.cpp Minefield.h silentscreen.h Screen.h
Minefield.o: Minefield.cpp Minefield.h types.h ncscreen.h dumbscreen.h silentscreen.h NCPlayer.h Player.h Screen.h AI.h

# DO NOT DELETE
