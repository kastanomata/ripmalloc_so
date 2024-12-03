CC = gcc
CFLAGS = -Wall -Wextra -g
# TARGET
SRCDIR = src
HEADDIR = header
BUILDDIR = build
BINDIR = bin

# SOURCES = main.c
# OBJECTS = main.o

BINS = $(BINDIR)/main
OBJECTS = $(BUILDDIR)/main.o $(BUILDDIR)/allocator.o

.phony: clean all

all: $(BINDIR)/main
	./$(BINDIR)/main

valgrind: $(BINDIR)/main
	valgrind ./$(BINDIR)/main

$(BINDIR)/main: $(OBJECTS)
	# Compiling binaries
	$(CC) $(CCOPTS) -o $@ $(OBJECTS)

$(BUILDDIR)/main.o:	$(SRCDIR)/main.c
	# Compiling object files
	$(CC) $(CCOPTS) -c -o $@  $<

$(BUILDDIR)/allocator.o: $(SRCDIR)/allocator.c
	# Compiling object files
	$(CC) $(CCOPTS) -c -o $@  $<


clean:
	rm -rf $(BUILDDIR)/*.o
	rm -rf $(BINDIR)/*
