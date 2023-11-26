# Compiler
CXX = g++

# Compiler flags
DEBUG = -g
CXXFLAGS = -std=c++17 -Wall $(DEBUG) 

# Linker flags
LDFLAGS = -lX11 -lGL -lpthread -lpng -lstdc++fs

# Target
TARGET = rayc

SRC_DIR = ./src

OBJ_DIR = .

SRC = $(SRC_DIR)/main.cpp $(SRC_DIR)/app.cpp

HDR = $(SRC_DIR)/app.h

# Object files
OBJ = $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

.PHONY: all run

all: $(TARGET)

$(TARGET): $(OBJ) $(HDR)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HDR) Makefile
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(OBJ) $(TARGET)