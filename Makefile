OPTS=-Wall -ggdb

.c.o:
	gcc $(OPTS) -c $<

OBJECTS = ai.o cmines.o ncscreen.o dumbscreen.o silentscreen.o ncplayer.o

all: $(OBJECTS)
	gcc $(OPTS) -lncurses $(OBJECTS) -o cmines

clean:
	rm -vf $(OBJECTS) cmines

ai.o: ai.c ai.h cmines.h Player.h
ncplayer.o: ncplayer.c ncplayer.h cmines.h Player.h
ncscreen.o: ncscreen.c ncscreen.h cmines.h Screen.h
dumbscreen.o: dumbscreen.c cmines.h dumbscreen.h Screen.h
silentscreen.o: silentscreen.c cmines.h silentscreen.h Screen.h
cmines.o: cmines.c cmines.h types.h ncscreen.h dumbscreen.h silentscreen.h ncplayer.h Player.h Screen.h ai.h

# DO NOT DELETE
