# paths
INCLUDE = ./include
MODULES = ./modules

# compiler
CC = gcc

# Compile options. 
CFLAGS =  -Werror -g -I$(INCLUDE) 
LDFLAGS = -lpthread -lm

# files .o
OBJS = fileserver.o  $(MODULES)/customer.o $(MODULES)/thread.o

# executable
EXEC = fileserver

# arguments
ARGS = 20 10 200 1

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(EXEC) log/*

run: $(EXEC)
	./$(EXEC) $(ARGS)

valgrind: $(EXEC)
	valgrind --error-exitcode=1 --leak-check=full --show-leak-kinds=all ./$(EXEC) $(ARGS)