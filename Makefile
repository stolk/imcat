imcat: imcat.c
	cc -D_POSIX_C_SOURCE=2 -std=c99 -Wall -g -o imcat imcat.c -lm

run: imcat
	./imcat ~/Desktop/*.png

clean:
	rm -f ./imcat

