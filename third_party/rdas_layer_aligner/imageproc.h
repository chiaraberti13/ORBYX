#pragma once

#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

class ImageProc {
public:
  struct HueShiftRule {
    int hMin;  // inclusive, 0–179
    int hMax;  // inclusive, 0–179
    int shift; // signed shift, wrap at 0–179
  };

private:
  // Specify the number of iterations.
  static constexpr int cNumOfIterations = 500;

  // Specify the threshold of the increment
  // in the correlation coefficient between two iterations
  static constexpr double cTerminationEps = 1e-7;

  static constexpr int cWarpMode = cv::MOTION_AFFINE;

public:
  static cv::Mat findMatrix(cv::Mat reference, cv::Mat target) {
    cv::TermCriteria criteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, cNumOfIterations, cTerminationEps);

    // Initialize the matrix to identity
    cv::Mat warpMatrix;
    if (cWarpMode == cv::MOTION_HOMOGRAPHY) {
      warpMatrix = cv::Mat::eye(3, 3, CV_32F);
    } else {
      warpMatrix = cv::Mat::eye(2, 3, CV_32F);
    }

    cv::findTransformECC(reference, target, warpMatrix, cWarpMode, criteria);
    return warpMatrix;
  }

  static cv::Mat shiftImageMatrix(float x, float y) {
    // Create 2x3 affine transform matrix for translation
    return (cv::Mat_<float>(2, 3) << 1, 0, x, 0, 1, y);
  }

  /// @brief Apply hue. OpenCV uses 0–179° (not 0–360°)
  /// @param image
  /// @param rules
  /// @param blend
  /// @return
  static cv::UMat hue(cv::UMat image, const std::vector<HueShiftRule>& rules, float blend);
};