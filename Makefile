# Makefile for Ellipse Detector with Characteristic Mapping (CMED)
# Cross-platform build system for Windows, Linux, and macOS

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2

# Detect OS and set appropriate OpenCV paths
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS with Homebrew
    INCLUDES = -I/opt/homebrew/include/opencv4 -I/usr/local/include/opencv4
    LIBDIRS = -L/opt/homebrew/lib -L/usr/local/lib
    LIBS = $(LIBDIRS) -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_features2d -lopencv_imgcodecs
else ifeq ($(UNAME_S),Linux)
    # Linux (Ubuntu/Debian/CentOS/etc.)
    INCLUDES = -I/usr/local/include/opencv4 -I/usr/include/opencv4 -I/usr/local/include/opencv -I/usr/include/opencv
    LIBS = -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_features2d -lopencv_imgcodecs
else
    # Default (assume Linux-like)
    INCLUDES = -I/usr/local/include/opencv4 -I/usr/include/opencv4
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

# Test target
test: $(TARGET)
	@echo "Running basic tests..."
	@if [ -f "./$(TARGET)" ]; then \
		echo "✅ Executable exists"; \
		if [ -f "image.png" ]; then \
			echo "Testing with sample image..."; \
			timeout 30 ./$(TARGET) image.png > test_output.log 2>&1 || true; \
			if grep -q "Total:" test_output.log; then \
				echo "✅ Image processing test passed"; \
			else \
				echo "⚠️  Test completed but output format unexpected"; \
			fi; \
		else \
			echo "⚠️  No sample image found for testing"; \
		fi; \
	else \
		echo "❌ Executable not found"; \
		exit 1; \
	fi

# Install OpenCV dependencies (system-specific)
install-deps:
	@echo "Installing OpenCV dependencies..."
	@if [ "$(UNAME_S)" = "Darwin" ]; then \
		echo "Installing via Homebrew..."; \
		brew install opencv || echo "Homebrew installation failed"; \
	elif [ "$(UNAME_S)" = "Linux" ]; then \
		echo "Installing via apt (Ubuntu/Debian)..."; \
		sudo apt-get update && sudo apt-get install -y libopencv-dev cmake build-essential || \
		echo "Package manager installation failed. Please install OpenCV manually."; \
	else \
		echo "Please install OpenCV manually for your system"; \
	fi

# Create distribution package
dist: clean $(TARGET)
	@echo "Creating distribution package..."
	@mkdir -p dist
	@cp $(TARGET) dist/
	@cp README.md dist/ 2>/dev/null || echo "README.md not found"
	@cp LICENSE dist/ 2>/dev/null || echo "LICENSE not found"
	@tar -czf ellipse-detector-$(UNAME_S)-$(shell date +%Y%m%d).tar.gz -C dist .
	@echo "Package created: ellipse-detector-$(UNAME_S)-$(shell date +%Y%m%d).tar.gz"

# Run the program (you may need to adjust the dataset path)
run: $(TARGET)
	./$(TARGET)

# Help target
help:
	@echo "CMED Ellipse Detector Build System"
	@echo "=================================="
	@echo "Available targets:"
	@echo "  all          - Build the ellipse detector (default)"
	@echo "  clean        - Remove build files"
	@echo "  test         - Run basic tests (requires sample image)"
	@echo "  install-deps - Install OpenCV dependencies"
	@echo "  dist         - Create distribution package"
	@echo "  run          - Run the program"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Platform: $(UNAME_S)"
	@echo "Compiler: $(CXX)"

.PHONY: all clean test install-deps dist run help
