OBJS	= mysh.o
SOURCE	= mysh.c 
OUT	= mysh
CC	 = gcc
FLAGS	 = -g -c -Wall
LFLAGS	 = 

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

words.o: words.c
	$(CC) $(FLAGS) mysh.c -std=c99

clean:
	rm -f $(OBJS) $(OUT)