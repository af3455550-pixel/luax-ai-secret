CXX ?= g++
CXXFLAGS = -std=c++20 -O3 -pthread -Wall -Wextra -I./Engine/Source
BIN_DIR = bin
TARGET = $(BIN_DIR)/ApexEngine
TARGET_EXE = $(BIN_DIR)/ApexEngine.exe

SRCS = Engine/Source/Programs/ApexEngineApp/main.cpp

all: $(TARGET) $(TARGET_EXE)

$(TARGET): $(SRCS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)
	@echo "Built $(TARGET) successfully."

$(TARGET_EXE): $(TARGET)
	@cp $(TARGET) $(TARGET_EXE)
	@echo "Created $(TARGET_EXE) executable binary."

run: $(TARGET)
	@./$(TARGET) 60

clean:
	rm -rf $(BIN_DIR)

.PHONY: all run clean
