OPTS=-Wall -ggdb

.c.o:
	gcc $(OPTS) -c $<

OBJECTS = ai.o cmines.o screen.o

all: $(OBJECTS)
	gcc $(OPTS) -lncurses $(OBJECTS) -o cmines

clean:
	rm -vf $(OBJECTS) cmines

ai.o: ai.c ai.h cmines.h Player.h
cmines.o: cmines.c cmines.h screen.h ai.h
screen.o: screen.c cmines.h screen.h

# DO NOT DELETE
