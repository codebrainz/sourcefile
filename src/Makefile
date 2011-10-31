#
# Makefile for libsourcefile.so
#

CC				= gcc
SF_CFLAGS	= -g -Wall -Werror \
							`pkg-config --cflags glib-2.0 gio-2.0 uchardet` \
							-DHAVE_UCHARDET -DHAVE_MAGIC
SF_LIBS		= `pkg-config --libs glib-2.0 gio-2.0` -lmagic

all: libsourcefile.so

libsourcefile.so: sourcefile.o
	$(CC) -shared $(SF_LIBS) -o $@ $^

sourcefile.o: sourcefile.c sourcefile.h
	$(CC) $(SF_CFLAGS) -c -fPIC -o $@ $<

clean:
	rm -f *.o *.so
