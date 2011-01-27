OPTS=-Wall -ggdb

.c.o:
	gcc $(OPTS) -c $<

OBJECTS = ai.o cmines.o ncscreen.o

all: $(OBJECTS)
	gcc $(OPTS) -lncurses $(OBJECTS) -o cmines

clean:
	rm -vf $(OBJECTS) cmines

ai.o: ai.c ai.h cmines.h Player.h
cmines.o: cmines.c cmines.h ncscreen.h Screen.h ai.h
ncscreen.o: ncscreen.c cmines.h ncscreen.h Screen.h

# DO NOT DELETE
