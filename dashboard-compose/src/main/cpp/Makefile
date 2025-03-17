# Compiler and flags
CC = gcc
CXX = g++
CFLAGS = -I.

# Source and object files
SRCS_C = can_interface.c
SRCS_CPP = can_processor.cpp
OBJS_C = can_interface.o
OBJS_CPP = can_processor.o

# Output executable
TARGET = test

# Default target
all: $(TARGET)
	$(RM) $(OBJS_C) $(OBJS_CPP)

# Compile C source file into object file
$(OBJS_C): $(SRCS_C)
	$(CC) $(CFLAGS) -c $(SRCS_C) -o $(OBJS_C)

# Compile C++ source file into object file
$(OBJS_CPP): $(SRCS_CPP)
	$(CXX) $(CXXFLAGS) -c $(SRCS_CPP) -o $(OBJS_CPP)

# Link object files to create the executable
$(TARGET): $(OBJS_C) $(OBJS_CPP)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS_C) $(OBJS_CPP)

# Clean up object files and the executable
clean:
	rm -f $(OBJS_C) $(OBJS_CPP) $(TARGET)

# Phony targets
.PHONY: all clean
