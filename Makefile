


CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++23 -Wno-missing-field-initializers -g 

RAYLIB_PATH = $(shell brew --prefix raylib)
RAYLIB_INCLUDE = -I$(RAYLIB_PATH)
RAYLIB_LIB = -L$(RAYLIB_PATH)/lib -lraylib

MACOS_FRAMEWORKS = -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL

INCLUDES = -I./src -I./lib $(RAYLIB_INCLUDE)

SOURCES = $(wildcard src/*.cpp) 
HEADERS = $(wildcard src/*.hpp) $(wildcard lib/*.hpp)

TARGET = bin 

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) -o $(TARGET) $(RAYLIB_LIB) $(MACOS_FRAMEWORKS)

clean:
	rm -f $(TARGET)

.PHONY: all clean