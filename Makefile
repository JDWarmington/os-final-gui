# Makefile for the CPU Core Scheduling Visualizer.
#
# Targets:
#   make           - build ./scheduler_viz
#   make run       - build and run
#   make clean     - remove build artifacts and the binary
#
# Required system packages on Debian/Ubuntu:
#   sudo apt install build-essential libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev
#
# Uses pkg-config for SDL2 flags so it works on both Debian/Ubuntu and
# Fedora-style distros without hard-coding include paths.

CXX        := g++
CXXSTD     := -std=c++17
WARNINGS   := -Wall -Wextra -Wpedantic
OPT        := -O2
CXXFLAGS   := $(CXXSTD) $(WARNINGS) $(OPT) $(shell pkg-config --cflags sdl2 SDL2_image SDL2_ttf)
LDFLAGS    := $(shell pkg-config --libs sdl2 SDL2_image SDL2_ttf)

SRC_DIR    := src
BUILD_DIR  := build
TARGET     := scheduler_viz

SOURCES    := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS    := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
DEPS       := $(OBJECTS:.o=.d)

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(DEPS)
