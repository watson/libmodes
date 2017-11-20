INCLUDE = ./include
CFLAGS ?= -O2 -g -Wall -W
LDLIBS += -lpthread -lm
CC ?= gcc
PROGNAME = adsb-test

all: adsb-test

%.o: %.c
	$(CC) -c $(CFLAGS) -I${INCLUDE} $^ -o $@

adsb-test: tests/test.o src/decoder.o
	$(CC) ${CFLAGS} ${LDLIBS} $^ -o $@

clean:
	rm -f *.o adsb-test
