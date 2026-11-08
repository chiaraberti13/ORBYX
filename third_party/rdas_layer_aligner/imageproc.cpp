#include "imageproc.h"


cv::UMat ImageProc::hue(cv::UMat image, const std::vector<HueShiftRule>& rules, float blend) {
  cv::Mat hsv;
  cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);

  std::vector<cv::Mat> hsvChannels;
  cv::split(hsv, hsvChannels);

  // Option 1
  for (int y = 0; y < hsvChannels[0].rows; y++) {
    uchar* hptr = hsvChannels[0].ptr<uchar>(y);
    for (int x = 0; x < hsvChannels[0].cols; x++) {
      uchar h = hptr[x];
      for (const auto& r : rules) {
        bool inRange = false;

        if (r.hMin <= r.hMax) {
          // normal range
          inRange = (h >= r.hMin && h <= r.hMax);
        } else {
          // wrap-around range, e.g. 170–10
          inRange = (h >= r.hMin || h <= r.hMax);
        }

        if (inRange) {
          int newHue = h + r.shift;

          // wrap around 0–179
          if (newHue < 0) {
            newHue += 180;
          }
          if (newHue >= 180) {
            newHue -= 180;
          }

          hptr[x] = (uchar)newHue;
          break; // apply first matching rule only
        }
      }
    }
  }

  // Option 2
  /*for (const auto& r : rules) {
    cv::Mat mask;

    if (r.hMin <= r.hMax) {
      cv::inRange(hsvChannels[0], r.hMin, r.hMax, mask);
    } else {
      // wrap-around range
      cv::Mat m1, m2;
      cv::inRange(hsvChannels[0], r.hMin, 179, m1);
      cv::inRange(hsvChannels[0], 0, r.hMax, m2);
      cv::bitwise_or(m1, m2, mask);
    }

    cv::add(hsvChannels[0], cv::Scalar(r.shift), hsvChannels[0], mask);
  }*/

  cv::UMat hsvShifted;
  cv::merge(hsvChannels, hsvShifted);

  cv::UMat shiftedBGR;
  cv::cvtColor(hsvShifted, shiftedBGR, cv::COLOR_HSV2BGR);

  // Alpha blend
  blend /= 100.0f;
  cv::UMat blended;
  cv::addWeighted(image, 1.0f - blend, shiftedBGR, blend, 0.0, blended);

  return blended;
}