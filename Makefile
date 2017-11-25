INCLUDE = ./include
CFLAGS ?= -O2 -g -Wall -W
LDLIBS += -lpthread -lm
CC ?= gcc

%.o: %.c
	$(CC) -c $(CFLAGS) -I${INCLUDE} $^ -o $@

all: tests/test.o src/decoder.o
	$(CC) ${CFLAGS} ${LDLIBS} $^ -o tests/test

test:
	if [ ! -d "tests/fixtures" ]; then \
		git clone --depth=1 https://github.com/watson/libasdb-test-fixtures.git tests/fixtures; \
	else \
		(cd tests/fixtures && git pull --depth=1 origin master); \
	fi
	tests/test tests/fixtures/dump.bin

clean:
	rm -f **/*.o tests/test
