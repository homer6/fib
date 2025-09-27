# Fibonacci Lookup Table Makefile

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++2b -O3 -march=native -mtune=native -flto -funroll-loops -fomit-frame-pointer -Wall -Wextra -pedantic
DEBUG_FLAGS = -g -O0 -DDEBUG

# Target executable name
TARGET = fib

# Source files
SOURCES = fib.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -flto -o $@ $^

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Debug build
debug: CXXFLAGS = $(DEBUG_FLAGS) -std=c++2b -Wall -Wextra -pedantic
debug: clean $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -f $(TARGET) $(OBJECTS)

# Rebuild everything
rebuild: clean all

.PHONY: all clean debug run rebuild