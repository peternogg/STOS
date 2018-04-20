SOURCES = $(wildcard *.c)
SLASMS  = $(SOURCES:.c=.sl)

all: $(SLASMS)
	slasm -dbg $^

%.sl: %.c
	stacklc -dbg -c $^
