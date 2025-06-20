# Compiler and flags
CC = gcc
CFLAGS = -O3 -flto
DEBUG_CFLAGS = -g -O0
LDFLAGS = -lSDL3 -lSDL3_image -lm

# Directories
BUILD_DIR = build

# Source files and object files
SRCS = main.c image_manipulations.c draw.c selection.c image.c utils.c
OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRCS))

# Target executable
TARGET = imagecutter

# Default target
all: $(TARGET)

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link object files into the target executable
$(TARGET): $(BUILD_DIR) $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

# Compile source files into object files in the build directory
$(BUILD_DIR)/%.o: %.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build
debug: CFLAGS := $(DEBUG_CFLAGS)
debug: $(TARGET)

# Clean up build files
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# PHONY targets (not real files)
.PHONY: all debug clean

# Support for parallel builds
ifneq ($(MAKEFLAGS),)
  ifeq ($(filter -j%, $(MAKEFLAGS)),)
    MAKEFLAGS += -j$(shell nproc)
  endif
endif
