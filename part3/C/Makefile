# Specifies gcc as compiler 
CC = gcc 

# Compiler flags: -g -Wall -pedantic -Werror -Wextra 
# -g to make file debuggable 
# -Wall turns on compiler warnings 
# -pedantic
# -Werror 
# -Wextra
CFLAGS = -g -Wall -pedantic -Werror -Wextra 

# name of executable file 
EXEC = part3
# name of c file to compile 
FILE = part3.c

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
