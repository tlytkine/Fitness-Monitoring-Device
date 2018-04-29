# Specifies gcc as compiler 
CC = gcc 

# Compiler flags: -g -Wall -pedantic -Werror 
# -g to make file debuggable 
# -Wall turns on compiler warnings 
# -pedantic
# -Werror 
# -Wextra
CFLAGS = -std=c99 -g -Wall -pedantic -Werror -lsqlite3

# name of executable file 
EXEC = part4
# name of c file to compile 
FILE = part4.c

all: $(EXEC)
# generates executable file part1 
$(EXEC): $(EXEC).c
		$(CC) $(CFLAGS) -o $(EXEC) $(FILE)
# command to generate executable file 
clean:
		$(RM) $(EXEC)
# command to run executable file 
run: $(EXEC)
	./$(EXEC)
