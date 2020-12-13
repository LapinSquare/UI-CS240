
# INSTRUCTIONS:
# Simply typing "make" will compile the necessary msh file. 
# File "msh.c" contains the shell "msh."  
# Type "make clean" to clear the created msh program. 
# Run "./msh" to execute shell.
#rm *.o

CC=gcc
RM=rm
CFLAGS=-I.

all: msh.c
	   $(CC) -g -o msh msh.c

#all: msh

#msh: msh.o
#	   $(CC) -o msh msh.o
       
#msh.o: msh.o
#	   $(CC) -c msh.c


emptyrule::
clean::
	   $(RM) msh

