DEBUG=1

ifeq ($(DEBUG),)
	OPTS=-Wall -O3
else
	OPTS=-Wall -ggdb -DDEBUG
endif

.c.o:
	g++ $(OPTS) -c $<

OBJECTS = AI.o Minefield.o ncscreen.o dumbscreen.o silentscreen.o NCPlayer.o

all: $(OBJECTS)
	g++ $(OPTS) -lncurses $(OBJECTS) -o cmines

clean:
	rm -vf $(OBJECTS) cmines

AI.o: AI.c AI.h Minefield.h Player.h
NCPlayer.o: NCPlayer.c NCPlayer.h Minefield.h Player.h
ncscreen.o: ncscreen.c ncscreen.h Minefield.h Screen.h
dumbscreen.o: dumbscreen.c Minefield.h dumbscreen.h Screen.h
silentscreen.o: silentscreen.c Minefield.h silentscreen.h Screen.h
Minefield.o: Minefield.c Minefield.h types.h ncscreen.h dumbscreen.h silentscreen.h NCPlayer.h Player.h Screen.h AI.h

# DO NOT DELETE
