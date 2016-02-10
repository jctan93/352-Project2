all: simVM test

simVM: simVM.c
	gcc -ansi -lpthread -lm -o simVM simVM.c
	
test: test.c
	gcc -ansi -lm -o test test.c
	
clean:
	rm -f *.o simVM test