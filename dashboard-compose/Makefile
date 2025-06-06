CC = gcc
CXX = g++
CFLAGS = -Wall
CXXFLAGS = -Wall

# Flags for linking a shared library
SOFLAGS = -shared -fPIC

# Include for the JNI library files. These contain the `<jni.h>` files.
JNI_PATH = -I$(JAVA_HOME)/include
LIB_JNI = $(JNI_PATH) $(JNI_PATH)/linux

# Include for the shared CAN libraries across the `firmware` project.
CAN_LIBRARY_PATH = ./../lib/can
LIB_CAN_ID = -I$(CAN_LIBRARY_PATH)

# All include flags for the dashboard.
INCLUDE = $(LIB_CAN_ID) $(LIB_JNI) 

# The source file directory for the C or C++ source files.
SRC = src/main/cpp
# The output file directory for the shared library. This is a subdirectory
# within the cached build files generated by the Kotlin/Java compiler. This
# should be the same as `$buildDir/cpp` in the `build.gradle.kts` file.
OUTPUT = build/cpp

# Get all source files.
# 	This matches all files in `SRC` that are C or C++ files.
C_SRCS := $(wildcard $(SRC)/*.c)
CPP_SRCS := $(wildcard $(SRC)/*.cpp)

# Create matching object files.
#	This matches all source files captured above, and generates output files
#	in `OUTPUT`.
C_OBJS := $(C_SRCS:$(SRC)/%.c=$(OUTPUT)/%.o)
CPP_OBJS := $(CPP_SRCS:$(SRC)/%.cpp=$(OUTPUT)/%.o)
OBJS := $(C_OBJS) $(CPP_OBJS)

# Shared library configuration.
#	All JNI shared libraries in Linux (`*.so` files) require the file name to
#	be `lib%.so`. However, when loading them in Java/Kotlin, the file must be
#	loaded as `%.so`.
LIBRARY_PREFIX = lib
LIBRARY = can_processor.so
TARGET = $(OUTPUT)/$(LIBRARY_PREFIX)$(LIBRARY)

# Compiles all required files.
#	Make the output directory if it doesn't already exist.
all: $(OUTPUT) $(TARGET)

$(OUTPUT):
	$(info Output directory does not exist; creating...)
	mkdir -p $(OUTPUT)

# Link object files to create the shared library file.
$(TARGET): $(OBJS)
	$(info Creating the shared library)
	$(CXX) $(SOFLAGS) $(OBJS) -o $(TARGET)

# Compiles and assembles all source files into object files.
$(OUTPUT)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) $(SOFLAGS) $(INCLUDE) -c $< -o $@

$(OUTPUT)/%.o: $(SRC)/%.cpp
	$(CXX) $(CXXFLAGS) $(SOFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -r $(OUTPUT)
