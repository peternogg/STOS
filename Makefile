DEBUG   = -dbg

.PHONY: all kernel user clean archive

all: kernel users

kernel: kernel.sl
	slasm $(DEBUG) kernel.sl

user: user.sl ulib.sl
	slasm $(DEBUG) user.sl ulib.sl

users: user1.sl user2.sl user3.sl ulib.sl
	slasm $(DEBUG) user1.sl ulib.sl
	slasm $(DEBUG) user2.sl ulib.sl
	slasm $(DEBUG) user3.sl ulib.sl
	mv user1.slb user.slb

%.sl: %.c
	stacklc $(DEBUG) -c $^

clean:
	-rm -f *.sl *.slb *.ast *.dbg

archive:
	tar -cv *.c *.h Makefile -f lab3_peter_higginbotham.tar
