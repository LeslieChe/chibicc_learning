CFLAGS=-std=c11 -g -fno-common
override CFLAGS += $(EXTRA_CFLAGS)
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

csub: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): csub.h

test: csub
	./test.sh

clean:
	rm -f csub *.o *~ tmp*

.PHONY: test clean
