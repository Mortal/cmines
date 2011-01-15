OPTS=-Wall -ggdb

.c.o:
	gcc $(OPTS) -c $<

OBJECTS = ai.o cmines.o

all: $(OBJECTS)
	gcc $(OPTS) -lncurses $(OBJECTS) -o cmines

clean:
	rm -v $(OBJECTS) cmines || true

ai.o: ai.c ai.h Player.h
cmines.o: cmines.c cmines.h ai.h

# DO NOT DELETE
