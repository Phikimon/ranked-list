DEPS=cache.h io.h list_unwrap.h minheap.h sort_file.h util.h
OBJS=cache.o io.o list_unwrap.o minheap.o sort_file.o util.o
CFLAGS=-std=c99 -c -O3 -g
LIBS := -lm

all: solve

solve: solve.o $(OBJS)
	$(CC) $(LFLAGS) $^ -o $@ $(LIBS)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) $<

clean:
	rm solve *.o 2> /dev/null
