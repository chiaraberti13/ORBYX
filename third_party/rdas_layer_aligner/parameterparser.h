#pragma once

#include <filesystem>
#include <list>
#include <map>
#include <string>

class ParameterParser {
private:
  struct ParameterData {
    ParameterData(const std::string arg, const std::string argShort, const std::string help)
      : argName(arg)
      , argNameShort(argShort)
      , helpText(help) {}
    std::string argName;
    std::string argNameShort;
    std::string helpText;
  };

public:
  enum class Mode { cGenerate, cCalibrate };

public:
  ParameterParser();
  void parseArgs(int argc, char** argv);
  std::string getHelp() const;

public: // getters
  Mode getMode() const;
  std::string getSatellite() const;
  std::filesystem::path getRDASDirectory() const;
  std::filesystem::path outputPath() const;

private:
  std::map<std::string, std::string> mArgs;
  std::list<ParameterData> mSettingsList;
};