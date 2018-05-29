DEBUG       = -dbg # Make this blank to disable debugging
CC          = stacklc
ASM         = slasm
PROGRAMS    = user.sl user1.sl user2.sl user3.sl shell.sl nothing.sl\
factorial.sl 2factorial.sl

.PHONY: all kernel user clean archive

all: kernel users

kernel: kernel.sl sched.sl mymalloc.sl queue.sl
	$(ASM) $(DEBUG) $^

users: ulib.sl $(PROGRAMS)
	$(ASM) $(DEBUG) user1.sl ulib.sl
	$(ASM) $(DEBUG) user2.sl ulib.sl
	$(ASM) $(DEBUG) user3.sl ulib.sl
	$(ASM) $(DEBUG) user.sl ulib.sl
	$(ASM) $(DEBUG) factorial.sl ulib.sl
	$(ASM) $(DEBUG) 2factorial.sl ulib.sl
	$(ASM) $(DEBUG) shell.sl ulib.sl
	$(ASM) $(DEBUG) nothing.sl ulib.sl

%.sl: %.c
	$(CC) $(DEBUG) -c $^

clean:
	-rm -f *.sl *.slb *.ast *.dbg

archive:
	tar -cv *.c *.h Makefile -f lab6_peter_higginbotham.tar
