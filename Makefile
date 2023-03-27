CC = gcc

OBJS = Main.o

EXEC = Main

ARGS = TestFile.txt 100 100

CFLAGS = -Wall -Werror -g

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) -lpthread

clean:
	rm -f $(OBJS) $(EXEC)

run: $(EXEC)
	./$(EXEC) $(ARGS)

valgrind: $(EXEC)
	valgrind ./$(EXEC) $(ARGS)

time: $(EXEC)
	time ./$(EXEC) $(ARGS)