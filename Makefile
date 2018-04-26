DEBUG   = -dbg

all: kernel user

kernel: kernel.sl
	slasm $(DEBUG) kernel.sl

user: user.sl ulib.sl
	slasm $(DEBUG) user.sl ulib.sl

%.sl: %.c
	stacklc $(DEBUG) -c $^

.PHONY: clean
clean:
	-rm -f *.sl *.slb *.ast *.dbg
