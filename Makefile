main: main.c log.c
	gcc -g -Wall -D_GNU_SOURCE -std=c2x -o main main.c
	gcc -g -Wall -D_GNU_SOURCE -std=c2x -o log log.c
