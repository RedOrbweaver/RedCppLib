# Compiler to use
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -Werror -std=c++20

# Source and header files
SOURCE = tests/tests.cpp
HEADER = RedCppLib.hpp

# Output binary name
TARGET = test

# Default target
all: $(TARGET)

# Rule to build the target
$(TARGET): $(SOURCE) $(HEADER)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Clean rule
clean:
	rm -f $(TARGET)