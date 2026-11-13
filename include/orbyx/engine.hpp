// ORBYX — GPL-3.0. Algorithm derived from Digitelektro/rdas_layer_aligner.
#pragma once
#include <array>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>
namespace orbyx {
struct RunOptions {
  std::string satellite;
  std::filesystem::path input, output, config;
  bool cpu = false;
  bool calibrate = false;
  std::string language = "en";
  // Explicit ECC working resolution; output transforms remain in native pixels.
  double calibrationScale = 1.0;
  // GUI opt-in: retain both aligned VIS products beside the byte-compatible
  // profile-ROI merge so they can be opened as editable compositor layers.
  // CLI/default output remains unchanged when false.
  bool editableParts = false;
  // Optional GUI/batch selection of relative MSUGS_VIS* product folders. An
  // empty list preserves the historical VIS1/VIS2 path byte-for-byte.
  std::vector<std::string> products;
};
using Log = std::function<void(const std::string&)>;
struct Inspection { int width, height, depth; };
struct CropRegion { int x=0, y=0, width=0, height=0; };
struct EditableMergeLayout {
  bool profileRoi = false;
  std::array<CropRegion,2> regions;
};
// All failures throw std::exception. The destination must not exist.
// The input images and input calibration profile are never modified.
void run(const RunOptions&, const Log& log = {});
std::string translate(const std::string& message, const std::string& language);
Inspection inspect(const std::filesystem::path& image);
// Resolve the same profile ROI decision used by generate for two already
// aligned representative channels. With no complete ROI pair, returns the full
// images. This is read-only and lets the GUI reproduce the historical merge as
// editable layers instead of flattening it first.
EditableMergeLayout editableMergeLayout(
    const std::filesystem::path& config, const std::string& satellite,
    const std::array<std::filesystem::path,2>& alignedChannels);
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
// MSU-GS RGB321 (CH3->R, CH2->G, CH1->B), legacy hue rules in floating
// point and 40% corrected blend. Preserves 8/16-bit depth; not calibrated
// true colour. Inputs unchanged; exclusive atomic PNG publication.
void composeNaturalColor(const std::array<std::filesystem::path, 3>& channels,
                         const std::filesystem::path& destination);
// Single-channel grayscale preset (M2-11): writes the chosen channel (index 0, 1
// or 2) as a neutral gray product (R=G=B), preserving 8/16-bit depth. Inputs
// unchanged; exclusive atomic PNG publication. Not calibrated true colour.
void composeSingleChannel(const std::array<std::filesystem::path, 3>& channels,
                          const std::filesystem::path& destination, int index);
// Curated six-JPEG desktop demo; measures channel registration and joins adjacent
// sx/dx halves. Output must be new; source images remain unchanged. Not RDAS input.
// exportScale (1..4) enlarges the final composite by interpolation on export;
// it adds pixels, not real detail, and is clamped so the output stays bounded.
void runImageDemo(const std::filesystem::path& input, const std::filesystem::path& output,
                  const Log& log = {}, int exportScale = 1);

}
