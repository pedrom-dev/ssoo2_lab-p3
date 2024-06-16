CC = g++
CFLAGS = -Wall -Wextra -std=c++17 -pthread -g
INCLUDES = -Iinclude
SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(patsubst src/%.cpp,obj/%.o,$(SOURCES))
TARGET = bin/main

all: $(TARGET)

$(TARGET): $(OBJECTS)
	mkdir -p bin
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

obj/%.o: src/%.cpp
	mkdir -p obj
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf obj bin user_directory/*

run:
	./$(TARGET)

.PHONY: all clean