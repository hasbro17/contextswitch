CC = gcc
CFLAGS = -march=native -O3 -D_GNU_SOURCE -std=c99 \
         -W -Wall -Werror
LDFLAGS = -pthread -lrt -lpthread


TARGETS = timectxsw timesyscall timetctxsw

all: bench

bench: $(TARGETS)
	./cpubench.sh

clean:
	rm -f $(TARGETS)

.PHONY: all bench
