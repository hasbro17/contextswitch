CC = gcc
CFLAGS = -march=native -O3 -D_GNU_SOURCE -std=c99 \
         -W -Wall -Werror
LDFLAGS = -pthread -lrt -lpthread


TARGETS = timectxsw timesyscall timetctxsw
TARGET = atimectxsw

all: atomic

bench: $(TARGETS)
	./cpubench.sh


atomic: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(LDFLAGS) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	rm -f $(TARGETS)

.PHONY: all bench
