#include "configstorage.h"

#include <fstream>

Config::Config(const std::filesystem::path& path)
  : mConfigPath(path) {
  std::ifstream ifstream(path);
  if (ifstream.is_open()) {
    ifstream >> mJsonConfig;
    ifstream.close();
  }
}

cv::Mat Config::getTransformMatrix(const std::string& index, const std::string& channel) {
  if (mJsonConfig[index].isNull() == true) {
    throw std::runtime_error("Selected index '" + index + "' config doesn't exist! Did you copy config from the repo's root directory? Is the satellite selected properly? (-s L2|L3|L4|M1|M2)");
  }

  auto satConf = mJsonConfig[index];
  if (satConf[channel].isNull() == true) {
    throw std::runtime_error("Selected index channel '" + channel + "' config doesn't exist");
  }

  auto channelConf = satConf[channel];
  float a = channelConf["a"].asFloat();
  float b = channelConf["b"].asFloat();
  float c = channelConf["c"].asFloat();
  float d = channelConf["d"].asFloat();
  float tx = channelConf["tx"].asFloat();
  float ty = channelConf["ty"].asFloat();

  return (cv::Mat_<float>(2, 3) << a, b, tx, c, d, ty);
}
void Config::setTransfromMatrix(const cv::Mat& matrix, const std::string& satellite, const std::string& channel) {
  Json::Value config(Json::objectValue);
  config["a"] = matrix.at<float>(0, 0);
  config["b"] = matrix.at<float>(0, 1);
  config["c"] = matrix.at<float>(1, 0);
  config["d"] = matrix.at<float>(1, 1);
  config["tx"] = matrix.at<float>(0, 2);
  config["ty"] = matrix.at<float>(1, 2);

  mJsonConfig[satellite][channel] = config;

  save();
}

bool Config::ROIExists(const std::string& key) {
  const Json::Value& sat = mJsonConfig[key];
  return sat.isMember("ROI");
}

cv::Rect Config::getROI(const std::string& satellite) {
  auto roi = mJsonConfig[satellite]["ROI"];
  return cv::Rect{roi["x"].asInt(), roi["y"].asInt(), roi["width"].asInt(), roi["height"].asInt()};
}

void Config::setROI(const std::string& satellite, const cv::Rect& rect) {
  Json::Value config(Json::objectValue);
  config["x"] = rect.x;
  config["y"] = rect.y;
  config["width"] = rect.width;
  config["height"] = rect.height;
  mJsonConfig[satellite]["ROI"] = config;

  save();
}

void Config::save() {
  int rc = 0;
  Json::StreamWriterBuilder jsonStreamWriter;
  jsonStreamWriter["indentation"] = "";
  std::string jsonString = Json::writeString(jsonStreamWriter, mJsonConfig);

  std::ofstream ofstream(mConfigPath, std::fstream::out | std::ios::trunc);
  if (ofstream.is_open()) {
    ofstream << jsonString;
    ofstream.close();
  } else {
    std::cout << "Failed to save config" << std::endl;
  }
}
