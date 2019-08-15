all: cli mht

cli : cli.c
	gcc -g -o cli cli.c -lcurl

mht : mht.c
	gcc -g -o mht mht.c -lmicrohttpd
