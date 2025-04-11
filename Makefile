prog: main.c
	gcc main.c helpers.c helpers.h -o prog -g

clean:
	rm -f prog