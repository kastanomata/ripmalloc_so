CC = gcc
CFLAGS = -Wall -Wextra 
# TARGET
SRCDIR = src
BLDDIR = build
BINDIR = bin

# SOURCES = main.c
# OBJECTS = main.o

BINS = ./bin/main

.phony: clean all

all: $(BINDIR)/main
	./$(BINDIR)/main

$(BINDIR)/main: $(BLDDIR)/main.o
	# Compiling binaries
	$(CC) $(CCOPTS) -o $@ $<

$(BLDDIR)/main.o:	$(SRCDIR)/main.c
	# Compiling object files
	$(CC) $(CCOPTS) -c -o $@  $<

clean:
	rm -rf $(BLDDIR)/*.o
	rm -rf $(BINDIR)/*
