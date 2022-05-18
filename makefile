CC=gcc
CFLAGS=-g -Wall

SRCDIR=src
INCLDIR=include
LIBDIR=lib

muxconf: $(SRCDIR)/muxconf.c
	$(CC) $(CFLAGS) $(SRCDIR)/muxconf.c -o muxconf

verbose: $(SRCDIR)/muxconf.c
	$(CC) $(CFLAGS) $(SRCDIR)/muxconf.c -o muxconf -DVERBOSE

test:
	./muxconf netlist mapfile output
clean:
	rm -f muxconf
