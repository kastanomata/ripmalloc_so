CC = gcc
CFLAGS = -Wall -Wextra -g -I$(HEADDIR) -I$(HEADDIR)/test
# TARGET
SRCDIR = src
HEADDIR = header
BUILDDIR = build
BINDIR = bin

BINS = $(BINDIR)/main
OBJECTS = $(BUILDDIR)/main.o $(BUILDDIR)/allocator.o \
          $(BUILDDIR)/test_allocator.o $(BUILDDIR)/double_linked_list.o \
          $(BUILDDIR)/test_double_linked_list.o

.phony: clean all valgrind

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

$(BUILDDIR)/double_linked_list.o: $(SRCDIR)/data_structures/double_linked_list.c
	# Compiling double linked list
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/test_double_linked_list.o: $(SRCDIR)/test/test_double_linked_list.c
	# Compiling double linked list tests
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -rf $(BUILDDIR)/*.o
	rm -rf $(BINDIR)/*