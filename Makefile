CC = gcc

OBJS = Main.o

EXEC = Main

ARGS = TestFile.log 100 100

CFLAGS = -Wall -Werror -g

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) -lpthread

clean:
	rm -f $(OBJS) $(EXEC) *.txt

run: $(EXEC)
	./$(EXEC) $(ARGS)

valgrind: $(EXEC)
	valgrind ./$(EXEC) $(ARGS)

time: $(EXEC)
	time ./$(EXEC) $(ARGS)