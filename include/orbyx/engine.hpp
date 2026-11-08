// ORBYX — GPL-3.0. Algorithm derived from Digitelektro/rdas_layer_aligner.
#pragma once
#include <array>
#include <filesystem>
#include <functional>
#include <string>
namespace orbyx {
struct RunOptions {
  std::string satellite;
  std::filesystem::path input, output, config;
  bool cpu = false;
  bool calibrate = false;
  std::string language = "en";
  // Explicit ECC working resolution; output transforms remain in native pixels.
  double calibrationScale = 1.0;
};
using Log = std::function<void(const std::string&)>;
struct Inspection { int width, height, depth; };
// All failures throw std::exception. The destination must not exist.
// The input images and input calibration profile are never modified.
void run(const RunOptions&, const Log& log = {});
std::string translate(const std::string& message, const std::string& language);
Inspection inspect(const std::filesystem::path& image);
// Copy one satellite's calibration entries to a new satellite id, writing a new
// profile that also preserves every original entry. The source profile is never
// modified and the destination must not already exist.
void duplicateProfile(const std::filesystem::path& source, const std::string& fromSatellite,
                      const std::string& toSatellite, const std::filesystem::path& destination);
// Compose a 3-channel RGB product (CH1->R, CH2->G, CH3->B) from three aligned
// grayscale channels of equal size and depth, preserving bit depth with no
// implicit conversion. Inputs are never modified; the destination must not
// exist. This is the composition foundation only: hue-corrected Natural Color
// and its validation on real scenes are not performed here.
void composeRgb(const std::array<std::filesystem::path, 3>& channels,
                const std::filesystem::path& destination);
}
