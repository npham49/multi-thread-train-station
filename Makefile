all: mts

mts: mts.c
	gcc mts.c -g -pthread -time -o mts

clean:
	-rm -rf *.o *.exe