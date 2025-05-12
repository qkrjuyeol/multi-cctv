#ifndef PCH_H
#define PCH_H

// prevent windows.h from defining min()/max() macros
#define NOMINMAX

// ── C++ STL ──
#include <initializer_list>   // optional

// ── OpenCV umbrella include ──
// this drags in core/types.hpp (InputArray, Mat, etc.), imgproc, imgcodecs, calib3d, …
#include <opencv2/opencv.hpp>

// ── MFC / Win32 headers ──
#include "framework.h"

#endif // PCH_H
