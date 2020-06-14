CC=gcc
CCFLAGS=-lm -lX11
CCFLAGS+=-Wall -g
# CCFLAGS+=-O3

SOURCES = $(wildcard *.c)
TARGETS = $(SOURCES:.c=.out)
HEADERS = $(wildcard *.h)

$(TARGETS): %.out: %.c $(HEADERS)
	$(CC) $< -o $@ $(CCFLAGS)

clean:
	rm -rf $(TARGETS)

