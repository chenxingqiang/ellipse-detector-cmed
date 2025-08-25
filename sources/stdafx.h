
#pragma once
#include "compatibility.h"
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/core/types_c.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>

#include <numeric>
#include <unordered_map>
#include <vector>

#include <time.h>
#include <iostream>
#include <fstream>
#include <math.h>

#include <sys/stat.h> 

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#elif defined(__linux__)
#include <dirent.h>
#include <unistd.h>
#include <sys/io.h>
#elif defined(__APPLE__)
#include <dirent.h>
#include <unistd.h>
#endif

using namespace std;using namespace cv;
