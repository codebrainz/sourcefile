
all: source-file-test

source-file-test: main.c
	gcc -g -Wall -Werror -I../src \
		`pkg-config --cflags --libs glib-2.0 gio-2.0 gtk+-2.0` \
		-o $@ $^ \
		-L../src -lsourcefile \
		-L/usr/local -luchardet \
		-L/usr -lmagic

clean:
	rm -f source-file-test
