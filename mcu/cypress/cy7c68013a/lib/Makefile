VER=devel
TAG=HEAD

.PHONY: all

all:
	make -C lib
	make -C examples

clean:
	make -C lib clean
	make -C examples clean

dist: all
	mkdir -p build	
	git archive --prefix=fx2lib/ $(TAG) > build/fx2-$(VER).tar
	tar -C .. -rf build/fx2-$(VER).tar \
		fx2lib/lib/fx2.lib \
	cat build/fx2-$(VER).tar | gzip > build/fx2-$(VER).tgz
