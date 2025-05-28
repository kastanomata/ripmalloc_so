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
          $(BUILDDIR)/double_linked_list.o \
          $(BUILDDIR)/buddy_allocator.o \
          $(BUILDDIR)/slab_allocator.o \
          $(BUILDDIR)/test_buddy_allocator.o \
          $(BUILDDIR)/test_slab_allocator.o \
          $(BUILDDIR)/test_time.o \
          $(BUILDDIR)/test_double_linked_list.o \

.PHONY: clean all valgrind verbose time

all: $(BINDIR)/main
	./$(BINDIR)/main

valgrind: $(BINDIR)/main
	valgrind  --track-origins=yes --show-leak-kinds=all --leak-check=full ./$(BINDIR)/main

massif: $(BINDIR)/main
	rm -f massif.out.*
	valgrind --tool=massif ./$(BINDIR)/main
	@msf_file=$$(ls massif.out.* | head -n 1); \
	if [ -f "$$msf_file" ]; then \
		ms_print "$$msf_file"; \
	else \
		echo "No massif output file found."; \
	fi

verbose: CFLAGS += -DVERBOSE
verbose: $(BINDIR)/main
	./$(BINDIR)/main

time: CFLAGS += -DTIME
time: $(BINDIR)/main
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

$(BUILDDIR)/buddy_allocator.o: $(SRCDIR)/buddy_allocator.c $(HEADDIR)/buddy_allocator.h $(HEADDIR)/allocator.h
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

$(BUILDDIR)/test_time.o: $(SRCDIR)/test/test_time.c $(HEADDIR)/test/test_time.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/test_buddy_allocator.o: $(SRCDIR)/test/test_buddy_allocator.c $(HEADDIR)/test/test_buddy_allocator.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILDDIR)/*.o $(BINDIR)/*