CHROOT_USER = torus
CHROOT_GROUP = $(CHROOT_USER)

CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS = -lcursesw
BINS = server client meta merge
OBJS = $(BINS:%=%.o)

all: tags $(BINS)

.o:
	$(CC) $(LDFLAGS) $< $(LDLIBS) -o $@

$(OBJS): torus.h

client.o: help.h

help.h:
	head -c 4096 torus.dat \
		| file2c -s -x 'static const uint8_t HELP_DATA[] = {' '};' \
		> help.h
	echo 'static const struct Tile *HELP = (const struct Tile *)HELP_DATA;' \
		>> help.h

tags: *.h *.c
	ctags -w *.h *.c

chroot.tar: server client
	mkdir -p root
	install -d -o root -g wheel \
	    root/bin \
	    root/home \
	    root/lib \
	    root/libexec \
	    root/usr \
	    root/usr/share \
	    root/usr/share/misc
	install -d -o $(CHROOT_USER) -g $(CHROOT_GROUP) root/home/$(CHROOT_USER)
	cp -p -f /libexec/ld-elf.so.1 root/libexec
	cp -p -f \
	    /lib/libc.so.7 \
	    /lib/libedit.so.7 \
	    /lib/libncurses.so.8 \
	    /lib/libncursesw.so.8 \
	    root/lib
	cp -a -f /usr/share/locale root/usr/share
	cp -p -f /usr/share/misc/termcap.db root/usr/share/misc
	cp -p -f /bin/sh root/bin
	install -o root -g wheel -m 555 server client root/bin
	tar -c -f chroot.tar -C root bin home lib libexec usr

clean:
	rm -f tags $(OBJS) $(BINS) chroot.tar
