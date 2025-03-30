# Compiler and flags
CC = gcc
CXX = g++
CFLAGS = -I.

# Java JNI paths
JNI_INCLUDE = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux

# Source and object files
SRCS_C = can_interface.c
SRCS_CPP = can_processor.cpp
SRCS_JNI = jni_bridge.c  # JNI source file
OBJS_C = can_interface.o
OBJS_CPP = can_processor.o
OBJS_JNI = jni_bridge.o

# Output executable and shared library
TARGET = test
JNI_BRIDGE = jni_bridge.so  # Shared library for JNI
JNI_OUTPUT_DIR=./../../../build/cpp
JNI_LIB = $(JNI_OUTPUT_DIR)/lib$(JNI_BRIDGE)

# Default target
all: $(TARGET) $(JNI_LIB)
	$(RM) $(OBJS_C) $(OBJS_CPP) $(OBJS_JNI)

# Compile C source file into object file
$(OBJS_C): $(SRCS_C)
	$(CC) $(CFLAGS) -c $(SRCS_C) -o $(OBJS_C)

# Compile C++ source file into object file
$(OBJS_CPP): $(SRCS_CPP)
	$(CXX) $(CXXFLAGS) -c $(SRCS_CPP) -o $(OBJS_CPP)

# Link object files to create the executable
$(TARGET): $(OBJS_C) $(OBJS_CPP)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS_C) $(OBJS_CPP)

# Compile JNI source file into object file
$(OBJS_JNI): $(SRCS_JNI)
	$(CC) $(CXXFLAGS) $(JNI_INCLUDE) -c $(SRCS_JNI) -o $(OBJS_JNI)

# Build JNI shared library
$(JNI_LIB): $(OBJS_JNI)
	mkdir -p $(JNI_OUTPUT_DIR)
	$(CC) -shared -fPIC $(JNI_INCLUDE) $(OBJS_JNI) -o $(JNI_LIB)

# Clean up object files and the executable
clean:
	rm -f $(OBJS_C) $(OBJS_CPP) $(OBJS_JNI) $(TARGET)

# Phony targets
.PHONY: all clean
