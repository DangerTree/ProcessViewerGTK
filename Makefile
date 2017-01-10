CFLAGS = -Wall -ggdb `pkg-config --cflags --libs gtk+-3.0` 
CC = gcc


testProcAccess : gtkProcStruct.o testProcAccess.c
	$(CC) testProcAccess.c gtkProcStruct.o -o $@ $(CFLAGS)


gtkProcStruct.o : gtkProcStruct.c
	$(CC) $< -c -o $@ $(CFLAGS)


.PHONY : clean

clean :
	rm -f testProcAccess
	rm -f testProcAccess.o
	rm -f gtkProcStruct.o

