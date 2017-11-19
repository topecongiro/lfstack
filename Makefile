CC				= gcc
CPPFLAGS	= -shared -fPIC
CFLAGS		= -std=gnu11 -O2 -Wall -pthread
LDFLAGS		= -ldl -pthread

.PHONY: clean run

run: test
	./test

test: main.c liblfstack.so
	$(CC) $(CFLAGS) main.c -o $@ -L . -llfstack

liblfstack.so: lfstack.c lfstack.h
	$(CC) $(CFLAGS) $(CPPFLAGS) lfstack.c -o $@ $(LDFLAGS)

clean:
	rm -rf liblfstack.so test
