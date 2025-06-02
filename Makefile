CC = gcc
CFLAGS = -Wall -Wextra -g -I$(HEADDIR) -I$(HEADDIR)/data_structures -I$(HEADDIR)/test -I$(HEADDIR)/helpers

# Directories
SRCDIR = src
HEADDIR = header
BUILDDIR = build
BINDIR = bin

# Targets
BINS = $(BINDIR)/main

DATA_STRUCTURES = $(BUILDDIR)/double_linked_list.o \
									$(BUILDDIR)/bitmap.o

HELPERS = $(BUILDDIR)/memory_manipulation.o \
					$(BUILDDIR)/time.o \
					$(BUILDDIR)/freeform.o \

TESTS = $(BUILDDIR)/test_slab_allocator.o \
				$(BUILDDIR)/test_buddy_allocator.o \
				$(BUILDDIR)/test_bitmap_buddy_allocator.o \
				$(BUILDDIR)/test_bitmap.o \
				$(BUILDDIR)/test_double_linked_list.o \


OBJECTS = $(BUILDDIR)/main.o \
          $(BUILDDIR)/slab_allocator.o \
          $(BUILDDIR)/buddy_allocator.o \
					$(BUILDDIR)/bitmap_buddy_allocator.o \

.PHONY: clean all valgrind verbose time

all: $(BINDIR)/main

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

# Define flags based on target names
VERBOSE_FLAG := $(if $(filter verbose,$(MAKECMDGOALS)),-DVERBOSE)
DEBUG_FLAG := $(if $(filter debug,$(MAKECMDGOALS)),-DDEBUG)
TIME_FLAG := $(if $(filter time,$(MAKECMDGOALS)),-DTIME)

# Combine all requested flags
REQUESTED_FLAGS := $(VERBOSE_FLAG) $(DEBUG_FLAG) $(TIME_FLAG)

# Main run rule
run: $(BINDIR)/main
	./$(BINDIR)/main

# Build binary with requested flags
$(BINDIR)/main: CFLAGS += $(REQUESTED_FLAGS)

# Shortcut targets that trigger the run
verbose debug time: run

.PHONY: run verbose debug time

$(BINDIR)/main: $(HELPERS) $(DATA_STRUCTURES) $(OBJECTS) $(TESTS) 
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $(HELPERS) $(DATA_STRUCTURES) $(OBJECTS) $(TESTS) -lm

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

$(BUILDDIR)/bitmap_buddy_allocator.o: $(SRCDIR)/bitmap_buddy_allocator.c $(HEADDIR)/bitmap_buddy_allocator.h $(HEADDIR)/allocator.h
	$(CC) $(CFLAGS) -c -o $@ $< 

# Data structures
$(BUILDDIR)/double_linked_list.o: $(SRCDIR)/data_structures/double_linked_list.c $(HEADDIR)/data_structures/double_linked_list.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/bitmap.o: $(SRCDIR)/data_structures/bitmap.c $(HEADDIR)/data_structures/bitmap.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Tests
$(BUILDDIR)/test_allocator.o: $(SRCDIR)/test/test_allocator.c $(HEADDIR)/test/test_allocator.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/test_double_linked_list.o: $(SRCDIR)/test/test_double_linked_list.c $(HEADDIR)/test/test_double_linked_list.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/test_bitmap.o: $(SRCDIR)/test/test_bitmap.c $(HEADDIR)/test/test_bitmap.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/test_slab_allocator.o: $(SRCDIR)/test/test_slab_allocator.c $(HEADDIR)/test/test_slab_allocator.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/test_buddy_allocator.o: $(SRCDIR)/test/test_buddy_allocator.c $(HEADDIR)/test/test_buddy_allocator.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/test_bitmap_buddy_allocator.o: $(SRCDIR)/test/test_bitmap_buddy_allocator.c $(HEADDIR)/test/test_bitmap_buddy_allocator.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Helpers 
$(BUILDDIR)/time.o: $(SRCDIR)/helpers/time.c $(HEADDIR)/helpers/time.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/memory_manipulation.o: $(SRCDIR)/helpers/memory_manipulation.c $(HEADDIR)/helpers/memory_manipulation.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/freeform.o: $(SRCDIR)/helpers/freeform.c $(HEADDIR)/helpers/freeform.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILDDIR)/*.o $(BINDIR)/*