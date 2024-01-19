CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: scheduler

scheduler: scheduler.o
	$(CC) $(CFLAGS) -o $@ $^

scheduler.o: scheduler.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o scheduler
