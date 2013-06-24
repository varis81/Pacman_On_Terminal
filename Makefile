#
# Makefile for pacman
#

all: pacman

clean:
	rm -f pacman *.o

pacman: pacman.c set_ticker.c 
	gcc -o pacman pacman.c set_ticker.c -lcurses

