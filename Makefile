SOURCES = $(wildcard *.c)
SLASMS  = $(SOURCES:.c=.sl)
DEBUG   = -dbg

all: kernel user

kernel: kernel.sl
	slasm $(DEBUG) $^

user: user.sl libc.sl
	slasm $(DEBUG) $^

%.sl: %.c
	stacklc -dbg -c $^

.PHONY: clean
clean:
	rm *.sl *.slb *.ast *.dbg
