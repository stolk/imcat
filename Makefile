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
	rm -f imcat_1.4.orig.tar.gz
	tar cvzf ../imcat_1.4.orig.tar.gz Makefile README.md stb_image.h imcat.c imcat.1 debian images

packageupload:
	debuild -S
	debsign ../imcat_1.4-1_source.changes
	dput ppa:b-stolk/ppa ../imcat_1.4-1_source.changes

