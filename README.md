# MyShell

## Overview
This is the UNIX shell made for the subject of a university. 

We can use pipes, redirection, C-c, C-z, 'bg', 'fg' and 'jobs'.

## Usage

    $ make
    $ ./shell
    
## Example

    $ myshell>ps
      PID TTY           TIME CMD
     3950 ttys000    0:00.06 -bash
     5187 ttys001    0:00.00 ./shell
     
    $ myshell>find . -name "*.c"
     ./builtin.c
     ./job.c
     ./list.c
     ./main.c
     ./parser.c
     ./tokenizer.c
     
    $ myshell>ls | grep .c
     builtin.c
     job.c
     list.c
     main.c
     parser.c
     tokenizer.c
     
    $ myshell>head -n 2 main.c > test.txt
    $ myshell>cat test.txt
     #include <stdlib.h>
     #include <string.h>
     
    $ myshell>sleep 100 &
    $ myshell>sleep 50 &
    $ myshell>jobs
     [0]: Running(background)	 sleep 100 &(5565)
     [1]: Running(background)	 sleep 50 &(5568)
    $ myshell>fg 0
     ^C5565: Terminated with signal:2
     myshell>jobs
     [0]: Running(background)	 sleep 50 &(5568)
    $ myshell>fg 0
     ^Zmyshell>jobs
     [0]: Stopped	 sleep 50 &(5568)
    $ myshell>bg 0
    $ myshell>jobs
     [0]: Running(background)	 sleep 50(5568)
    $ myshell>kill 5568
     5568: Terminated with signal:15
     [0]	Completed	 sleep 50 & (pgid:5568)