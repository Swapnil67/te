PKGS=sdl2
CFLAGS=-Wall -Wextra -std=c11 -pedantic -ggdb `pkg-config --cflags $(PKGS)`
LIBS=`pkg-config --libs $(PKGS)` -lm

te: main.c
	$(CC) $(CFLAGS) -o te main.c la.c editor.c $(LIBS)