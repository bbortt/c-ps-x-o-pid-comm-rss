#!/bin/sh
gcc ps.c -o ps || { echo "C code failed to compile"; exit 1; }
timeout -s SIGKILL 1s ./ps > raw.out || { echo "Your 'ps' command failed to run" ; exit 1 ; }
cat raw.out | grep -v ps | grep -v timeout | sort -n > have.out
/bin/ps x -o pid,comm,rss > raw2.out || { echo "System 'ps' failed to run"; exit 2; }
cat raw2.out | grep -v "COMMAND" | grep -v ps | sort -n > want.out
diff -w want.out have.out > delta.txt || { echo "Fail, delta is:" ; cat delta.txt ; exit 1; }
echo "Test passed"
