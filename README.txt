The shell:
    Included is a simple shell which can launch the user programs. Typing 'ls' 
    will list the programs the shell knows about. You can also add more by placing
    a .slb file in the same directory as the shell executable and typing its name
    at the prompt. Typing 'help' will give more detail.

How to witness interleaving
    Executing 'user1' at the prompt launches a series of programs which run
    at the same time. User1 will print while user2 does the same, and you can
    see that near the beginning of the output stream. The same behavior is
    visible for user1 and user3 at the top of the second half. Also, running
    '2factorial' will show the output of two factorial programs running at
    the same time. Comparing that with 'factorial' should show interleaving.
