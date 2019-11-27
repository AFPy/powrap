all: wrap.so wrap

wrap: wrap.c
	cc wrap.c -lunistring -o wrap

wrap.so: wrap.o
	cc -shared wrap.o -o wrap.so  -lunistring

wrap.o: wrap.c
	cc -c wrap.c -lunistring -fPIC -o wrap.o

.PHONY: clean
clean:
	rm -f wrap wrap.o wrap.so
