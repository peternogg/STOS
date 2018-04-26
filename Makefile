DEBUG   = -dbg

.PHONY: all kernel user clean archive

all: kernel user

kernel: kernel.sl
	slasm $(DEBUG) kernel.sl

user: user.sl ulib.sl
	slasm $(DEBUG) user.sl ulib.sl

%.sl: %.c
	stacklc $(DEBUG) -c $^

clean:
	-rm -f *.sl *.slb *.ast *.dbg

archive:
	tar -cv *.c *.h Makefile -f lab3_peter_higginbotham.tar
