CC=gcc
CFLAGS=
DEPS=

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

cpi2hex: src/cpi2hex.o
	gcc -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f src/*.o
