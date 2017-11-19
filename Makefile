CC				= gcc
CPPFLAGS	= -shared -fPIC
CFLAGS		= -std=gnu11 -O2 -Wall -pthread
LDFLAGS		= -ldl -pthread

.PHONY: clean run libftebr

run: test libftebr
	@LD_LIBRARY_PATH=./:../target:$LD_LIBRARY_PATH ./test

libftebr:
	cd .. && make target/libftebr.so

test: main.c liblfstack.so
	$(CC) $(CFLAGS) main.c -o $@ -L . -llfstack -L ../target -lftebr -I../include

liblfstack.so: lfstack.c lfstack.h
	$(CC) $(CFLAGS) $(CPPFLAGS) lfstack.c -o $@ $(LDFLAGS) -I../include

clean:
	rm -rf liblfstack.so test
