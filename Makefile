imcat: imcat.c
	cc -Wall -g -o imcat imcat.c -lm

run: imcat
	./imcat ~/Desktop/*.png

clean:
	rm -f ./imcat

