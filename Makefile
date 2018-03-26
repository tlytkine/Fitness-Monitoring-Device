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
EXEC = part1.o
# name of c file to compile 
FILE = part1.c

all: $(EXEC)

$(TARGET): $(TARGET).c
		$(CC) $(CFLAGS) -o $(EXEC) $(FILE)
clean:
		$(RM) $(EXEC)