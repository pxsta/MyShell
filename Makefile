CC = gcc
#CFLAGS = -g -O0 -rdynamic -Wall
RM = rm -f
OBJS = main.o list.o parser.o tokenizer.o job.o builtin.o

#t : $(OBJS)
#	$(CC) $(OBJS)

shell : $(OBJS)
	$(CC) -o $@ $(OBJS)

clean :
	$(RM) __rm_food__ *.o
