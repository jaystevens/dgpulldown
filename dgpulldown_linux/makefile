dgpulldown: dgpulldown.c
	gcc dgpulldown.c -O3 -o dgpulldown

static:
	gcc dgpulldown.c -static -O3 -o dgpulldown

native:
	gcc dgpulldown.c -march=native -O3 -o dgpulldown

install: dgpulldown
	cp dgpulldown $(HOME)/bin

clean:
	rm dgpulldown
