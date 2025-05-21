CC = gcc
CFLAGS = -Wall -Wextra -g -I$(HEADDIR) -I$(HEADDIR)/test
# TARGET
SRCDIR = src
HEADDIR = header
BUILDDIR = build
BINDIR = bin

BINS = $(BINDIR)/main
OBJECTS = $(BUILDDIR)/main.o $(BUILDDIR)/allocator.o $(BUILDDIR)/test_allocator.o

.phony: clean all

all: $(BINDIR)/main
	./$(BINDIR)/main

valgrind: $(BINDIR)/main
	valgrind ./$(BINDIR)/main

$(BINDIR)/main: $(OBJECTS)
	# Compiling binaries
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

$(BUILDDIR)/main.o: $(SRCDIR)/main.c
	# Compiling object files
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/allocator.o: $(SRCDIR)/allocator.c
	# Compiling object files
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/test_allocator.o: $(SRCDIR)/test/test_allocator.c
	# Compiling test object files
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILDDIR)/*.o
	rm -rf $(BINDIR)/*