cmake_minimum_required(VERSION 3.16)
project(EllipseDetectorCMED VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -O2)
endif()

# Find OpenCV - handle vcpkg installation
if(VCPKG_TOOLCHAIN)
    # For vcpkg, try to find OpenCV4 first
    message(STATUS "Using vcpkg toolchain for OpenCV")
    
    # Try different OpenCV package names that vcpkg might use
    find_package(opencv4 CONFIG QUIET)
    if(opencv4_FOUND)
        message(STATUS "Found opencv4 package")
        # Use target names that opencv4 provides
        set(OpenCV_LIBS opencv_core opencv_imgproc opencv_highgui opencv_features2d opencv_imgcodecs)
        set(OpenCV_INCLUDE_DIRS ${opencv4_INCLUDE_DIRS})
        set(OpenCV_VERSION ${opencv4_VERSION})
    else()
        # Try standard OpenCV name
        find_package(OpenCV CONFIG QUIET)
        if(OpenCV_FOUND)
            message(STATUS "Found standard OpenCV package")
        else()
            # Last resort: try with pkg-config style
            message(STATUS "Standard packages not found, trying with REQUIRED flag")
            find_package(OpenCV CONFIG REQUIRED)
        endif()
    endif()
else()
    # For system packages, use standard find
    find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui features2d imgcodecs)
endif()

# Print OpenCV info for debugging
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")

# Additional debugging for vcpkg
if(VCPKG_TOOLCHAIN)
    message(STATUS "VCPKG_TOOLCHAIN is ON")
    if(opencv4_FOUND)
        message(STATUS "Using opencv4 package from vcpkg")
        message(STATUS "opencv4_VERSION: ${opencv4_VERSION}")
        message(STATUS "opencv4_INCLUDE_DIRS: ${opencv4_INCLUDE_DIRS}")
    else()
        message(STATUS "Using standard OpenCV package from vcpkg")
    endif()
endif()

# Include directories
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(sources)

# Source files
set(SOURCES
    sources/Main.cpp
    sources/CNEllipseDetector.cpp
    sources/common.cpp
    sources/tools.cpp
)

# Header files
set(HEADERS
    sources/CNEllipseDetector.h
    sources/common.h
    sources/tools.h
    sources/stdafx.h
    sources/compatibility.h
    sources/dirent.h
)

# Create executable
add_executable(ellipse_detector ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(ellipse_detector ${OpenCV_LIBS})

# Platform-specific configurations
if(WIN32)
    # Windows specific settings
    set_target_properties(ellipse_detector PROPERTIES
        WIN32_EXECUTABLE FALSE
        OUTPUT_NAME ellipse_detector
    )
elseif(APPLE)
    # macOS specific settings
    set_target_properties(ellipse_detector PROPERTIES
        OUTPUT_NAME ellipse_detector
    )
elseif(UNIX)
    # Linux specific settings
    set_target_properties(ellipse_detector PROPERTIES
        OUTPUT_NAME ellipse_detector
    )
endif()

# Installation
install(TARGETS ellipse_detector
    RUNTIME DESTINATION bin
)

# Install additional files
install(FILES README.md DESTINATION . OPTIONAL)
install(FILES LICENSE DESTINATION . OPTIONAL)

# Package configuration
set(CPACK_PACKAGE_NAME "EllipseDetectorCMED")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cross-platform Ellipse Detector with Characteristic Mapping")
set(CPACK_PACKAGE_VENDOR "CMED Project")

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
