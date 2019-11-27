all: wrap.so

wrap.so: wrap.o
	cc -shared wrap.o -o wrap.so  -lunistring

wrap.o: wrap.c
	cc -c wrap.c -lunistring -fPIC -o wrap.o

.PHONY: clean
clean:
	rm wrap.o wrap.so
