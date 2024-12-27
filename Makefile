CFLAGS?=-Os -pedantic -Wall

all:
	$(CC) $(CFLAGS) fswm.c -lX11 -o fswm

clean:
	rm -f fswm
