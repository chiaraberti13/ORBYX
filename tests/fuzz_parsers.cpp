// ORBYX — GPL-3.0. libFuzzer harness for the untrusted-input parsing surface.
//
// Exercises the hardened paths reachable from attacker-controlled bytes through
// the public core API: the PNG snapshot + IHDR preflight + decode reached via
// inspect(), and the localisation catalogue reached via translate(). Every
// documented failure is an exception, so catching std::exception is expected;
// the fuzzer's purpose is to surface memory-safety or undefined-behaviour bugs
// (via -fsanitize=fuzzer,address,undefined), not to assert a return value.
//
// Build: cmake -S . -B fuzz -DORBYX_BUILD_GUI=OFF -DORBYX_FUZZ=ON \
//        -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug
// Native distribution codecs (OpenCV) are not instrumented; a crash inside them
// still aborts the process and is caught, but their internals are out of scope.
#include "orbyx/engine.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif

namespace {
namespace fs = std::filesystem;

const fs::path& scratch_png() {
  static const fs::path path = fs::temp_directory_path() /
      ("orbyx-fuzz-" + std::to_string(static_cast<long>(::getpid())) + ".png");
  return path;
}
}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // 1) Untrusted PNG bytes through the public inspect() entry: this drives the
  //    bounded snapshot read, the hand-rolled IHDR validation and dimension
  //    budget, and (for inputs that survive preflight) the decoder.
  {
    std::ofstream out(scratch_png(), std::ios::binary | std::ios::trunc);
    if (out) {
      if (size)
        out.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
      out.close();
      try {
        orbyx::inspect(scratch_png());
      } catch (const std::exception&) {
      }
    }
  }
  // 2) Localisation must accept arbitrary bytes as a diagnostic message without
  //    misbehaving on prefix matching or UTF boundaries.
  try {
    const std::string message(reinterpret_cast<const char*>(data), size);
    orbyx::translate(message, "it");
    orbyx::translate(message, "en");
  } catch (const std::exception&) {
  }
  return 0;
}
