all: myELF

myELF: myELF.o 
	gcc -g -m32 -Wall -o myELF myELF.o

myELF.o : task1.c
	gcc -g -Wall -m32  -c -o myELF.o  task1.c
	
.PHONY: clean

clean: 
	rm -f *.o myELF

