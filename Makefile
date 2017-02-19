CC = g++
CFLAGS = -std=gnu++11 -lpthread -Og
DEPS = silicon.h exceptions.h
ODIR = obj
_OBJS = main.o silicon.o

OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

$(ODIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

silicon: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm obj/main.o obj/silicon.o silicon
