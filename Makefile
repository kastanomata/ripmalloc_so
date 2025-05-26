CC = gcc
CFLAGS = -Wall -Wextra -g -I$(HEADDIR) -I$(HEADDIR)/data_structures -I$(HEADDIR)/test

# Directories
SRCDIR = src
HEADDIR = header
BUILDDIR = build
BINDIR = bin

# Targets
BINS = $(BINDIR)/main
OBJECTS = $(BUILDDIR)/main.o \
          $(BUILDDIR)/slab_allocator.o \
          $(BUILDDIR)/double_linked_list.o \
          $(BUILDDIR)/test_slab_allocator.o
        #   $(BUILDDIR)/allocator.o \
          $(BUILDDIR)/test_double_linked_list.o \

.PHONY: clean all valgrind verbose

all: $(BINDIR)/main
	./$(BINDIR)/main

valgrind: $(BINDIR)/main
	valgrind --leak-check=full ./$(BINDIR)/main
	
verbose: $(BINDIR)/main
	$(CC) $(CFLAGS) -DVERBOSE -c $(SRCDIR)/slab_allocator.c -o $(BUILDDIR)/slab_allocator.o
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)
	./$(BINDIR)/main

$(BINDIR)/main: $(OBJECTS)
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

# Main and core components
$(BUILDDIR)/main.o: $(SRCDIR)/main.c $(HEADDIR)/main.h
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/allocator.o: $(SRCDIR)/allocator.c $(HEADDIR)/allocator.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/slab_allocator.o: $(SRCDIR)/slab_allocator.c $(HEADDIR)/slab_allocator.h $(HEADDIR)/allocator.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/double_linked_list.o: $(SRCDIR)/data_structures/double_linked_list.c $(HEADDIR)/data_structures/double_linked_list.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Tests
$(BUILDDIR)/test_allocator.o: $(SRCDIR)/test/test_allocator.c $(HEADDIR)/test/test_allocator.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/test_double_linked_list.o: $(SRCDIR)/test/test_double_linked_list.c $(HEADDIR)/test/test_double_linked_list.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/test_slab_allocator.o: $(SRCDIR)/test/test_slab_allocator.c $(HEADDIR)/test/test_slab_allocator.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILDDIR)/*.o $(BINDIR)/*