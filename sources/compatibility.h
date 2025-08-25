#pragma once

// Cross-platform directory creation
#ifdef _WIN32
#include <direct.h>
#define MKDIR(dir) _mkdir(dir)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR(dir) mkdir(dir, 0755)
#endif

// OpenCV compatibility macros
#ifdef CV_BGR2GRAY
// If CV_BGR2GRAY is already defined, use it
#else
#define CV_BGR2GRAY COLOR_BGR2GRAY
#endif

// Replace deprecated C API functions
#define cvSaveImage(filename, image) imwrite(filename, *image)
#define cvDestroyWindow(name) destroyWindow(name)
#define cvShowImage(name, image) imshow(name, *image)
#define cvCreateImage(size, depth, channels) new Mat(size, CV_8UC(channels))
#define cvSobel(src, dst, xorder, yorder, aperture_size) Sobel(src, dst, CV_32F, xorder, yorder, aperture_size)
#define CV_CANNY_L2_GRADIENT 2
#define cvNamedWindow(name, flags) namedWindow(name, flags)
#define cvWaitKey(delay) waitKey(delay)
#define cvMoveWindow(name, x, y) moveWindow(name, x, y)

// OpenCV constants compatibility
#define CV_WINDOW_NORMAL WINDOW_NORMAL
#define CV_WINDOW_AUTOSIZE WINDOW_AUTOSIZE
#define CV_INTER_CUBIC INTER_CUBIC
#define CV_AA LINE_AA

// IplImage compatibility (if needed)
// Note: Modern OpenCV code should use cv::Mat instead of IplImage
