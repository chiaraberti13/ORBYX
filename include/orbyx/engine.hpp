// ORBYX — GPL-3.0. Algorithm derived from Digitelektro/rdas_layer_aligner.
#pragma once
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
};
using Log = std::function<void(const std::string&)>;
struct Inspection { int width, height, depth; };
// All failures throw std::exception. The destination must not exist.
// The input images and input calibration profile are never modified.
void run(const RunOptions&, const Log& log = {});
std::string translate(const std::string& message, const std::string& language);
Inspection inspect(const std::filesystem::path& image);
}
