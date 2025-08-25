# Makefile for Ellipse Detector with Characteristic Mapping
# This project implements the CMED algorithm

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2

# Detect OS and set appropriate OpenCV paths
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS with Homebrew
    INCLUDES = -I/opt/homebrew/include/opencv4
    LIBS = -L/opt/homebrew/lib -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_features2d -lopencv_imgcodecs -lopencv_videoio
else
    # Linux
    INCLUDES = -I/usr/local/include/opencv4 -I/usr/include/opencv4 -I/usr/local/include/opencv -I/usr/include/opencv
    LIBS = -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_features2d -lopencv_imgcodecs
endif

# Source files
SOURCES = sources/Main.cpp sources/CNEllipseDetector.cpp sources/common.cpp sources/tools.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Target executable
TARGET = ellipse_detector

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LIBS)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Install dependencies (for Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install -y libopencv-dev build-essential

# Install dependencies (for macOS with Homebrew)
install-deps-mac:
	brew install opencv

# Run the program (you may need to adjust the dataset path)
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean install-deps install-deps-mac run
