CC=gcc 
LIBS=-lcurses
CFLAGS=-Wfatal-errors -Wall -O0 -ggdb -DDEBUG

EXE = sockblaster
OBJS = main.o key.o

${EXE}: ${OBJS}
	${CC} -o ${EXE}  ${CFLAGS} ${LIBS} ${OBJS}

main.o: main.c key.h
	${CC} ${CFLAGS} -O -c main.c 

key.o: key.c key.h
	${CC} ${CFLAGS} -O -c key.c

clean:
	rm -f ${EXE} ${OBJS} *~
	
                     
