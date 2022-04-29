# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:


# the build target executable:
TARGET = SIM
FILE = Project_1_Cache

all: $(TARGET)

$(TARGET): $(FILE).c
	$(CC) -o $(TARGET) $(FILE).c

clean:
	$(RM) $(TARGET)