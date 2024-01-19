CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = -lm

SRCS = myshell.c
OBJS = $(SRCS:.c=.o)
EXECUTABLE = myshell

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(CC) $(OBJS) -o $(EXECUTABLE) $(LDFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJS) $(EXECUTABLE)
