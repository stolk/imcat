imcat: imcat.c
	$(CC) -D_POSIX_C_SOURCE=2 -std=c99 -Wall -g -o imcat imcat.c -lm

run: imcat
	./imcat ~/Desktop/*.png

clean:
	rm -f ./imcat

install: imcat
	install -d ${DESTDIR}/usr/bin
	install -m 755 imcat ${DESTDIR}/usr/bin/

uninstall:
	sudo rm -f ${DESTDIR}/usr/bin/imcat

tarball:
	tar cvzf ../imcat_1.5.orig.tar.gz Makefile README.md stb_image.h imcat.c imcat.1 debian images

packageupload:
	debuild -S
	debsign ../imcat_1.5-1_source.changes
	dput ppa:b-stolk/ppa ../imcat_1.5-1_source.changes

