DEBUG   = -dbg
CC      = stacklc
ASM     = slasm

.PHONY: all kernel user clean archive

all: kernel users user

kernel: kernel.sl sched.sl mymalloc.sl queue.sl
	$(ASM) $(DEBUG) $^

user: user.sl ulib.sl
	$(ASM) $(DEBUG) $^

users: user1.sl user2.sl user3.sl ulib.sl shell.sl nothing.sl
	$(ASM) $(DEBUG) user1.sl ulib.sl
	$(ASM) $(DEBUG) user2.sl ulib.sl
	$(ASM) $(DEBUG) user3.sl ulib.sl
	$(ASM) $(DEBUG) shell.sl ulib.sl
	$(ASM) $(DEBUG) nothing.sl ulib.sl

%.sl: %.c
	$(CC) $(DEBUG) -c $^

clean:
	-rm -f *.sl *.slb *.ast *.dbg

archive:
	tar -cv *.c *.h Makefile -f lab3_peter_higginbotham.tar
