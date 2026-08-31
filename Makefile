CXX = g++
CXXFLAGS = -std=c++17 -I src -O3 -march=native -Wall -Wextra -DFIRELINE_HAS_RAYLIB=0
LDFLAGS = -lpthread -lm

SRC = src/main.cpp
TARGET = fireline

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET) ($(shell du -h $(TARGET) | cut -f1))"

debug:
	$(CXX) -std=c++17 -I src src/main.cpp -o $(TARGET)_debug -lpthread -lm -g -O1 -DDEBUG

test: $(TARGET)
	./$(TARGET) --test

demo: $(TARGET)
	./$(TARGET) --demo

credits: $(TARGET)
	./$(TARGET) --credits

clean:
	rm -f $(TARGET) $(TARGET)_debug
	rm -rf build/

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)/usr/local/bin/fireline

# Graphical build with raylib (if available)
raylib:
	$(CXX) -std=c++17 -I src src/main.cpp -o $(TARGET)_raylib -lpthread -lm -lraylib -lGL -ldl -DFIRELINE_HAS_RAYLIB=1 -O3
	@echo "Graphical build complete"

.PHONY: all debug test demo credits clean install raylib
