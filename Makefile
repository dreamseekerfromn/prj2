CC = gcc

.c.o:
	gcc -g -c $<

all: master slave

master: main.o loglib.o
	$(CC) -o master main.o loglib.o

main.o: main.c log.h pcb.h
	gcc -c main.c

loglib.o: loglib.c log.h
	gcc -c loglib.c

slave: slave.o loglib.o
	$(CC) -o slave slave.o loglib.o

slave.o: slave.c log.h pcb.h
	$(CC) -c slave.c

clean:
	rm *.o
