// GPL-3.0. Coverage-guided PNG/profile/generation harness on synthetic inputs.
// Distribution OpenCV/JsonCpp libraries are not themselves instrumented.
#include "orbyx/engine.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core.hpp>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>

namespace {
namespace fs=std::filesystem;
struct Fixture {
 fs::path root;
 Fixture() {
  std::string pattern=(fs::temp_directory_path()/"orbyx-fuzz-XXXXXX").string();
  std::vector<char> buffer(pattern.begin(),pattern.end());buffer.push_back(0);
  char* created=::mkdtemp(buffer.data());if(!created)std::abort();root=created;
  for(int vis=1;vis<=2;++vis) {
   const auto dir=root/"input"/("MSUGS_VIS"+std::to_string(vis));fs::create_directories(dir);
   for(int channel=1;channel<=3;++channel) {
    cv::Mat frame=cv::Mat::zeros(3622,8,CV_8U);
    const int offset=channel==2?3606:channel==3?1804:0;
    for(int y=0;y<16;++y)for(int x=0;x<8;++x)frame.at<unsigned char>(y+offset,x)=static_cast<unsigned char>(20+x*3+y*5);
    if(!cv::imwrite((dir/("MSUGS-VIS-"+std::to_string(channel)+".png")).string(),frame))std::abort();
   }
   std::ofstream(dir/"product.cbor")<<"synthetic-fuzz-cbor-"<<vis;
  }
 }
 ~Fixture(){std::error_code ignored;fs::remove_all(root,ignored);}
};
void write(const fs::path& path,const uint8_t* data,size_t size) {
 std::ofstream out(path,std::ios::binary|std::ios::trunc);
 if(size)out.write(reinterpret_cast<const char*>(data),static_cast<std::streamsize>(size));
 if(!out)std::abort();
}
}
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data,size_t size) {
 static Fixture fixture;
 // Every byte string still exercises PNG preflight/decoding and localisation.
 const auto image=fixture.root/"untrusted.png";
 write(image,data,size);
 try {orbyx::inspect(image);}catch(const std::exception&){}
 // Structured seeds enter the real bounded JSON reader and generation pipeline:
 // affine validation, ROI dimensions, transaction staging and output writing.
 // Fixed small image dimensions keep valid-profile mutations inexpensive.
 if(size && (data[0]=='{' || data[0]=='[')) {
  const auto profile=fixture.root/"profile.json",output=fixture.root/"output";
  write(profile,data,size);
  try {orbyx::run({"L2",fixture.root/"input",output,profile,true,false});}
  catch(const std::exception&){}
  std::error_code ignored;fs::remove_all(output,ignored);
 }
 try {
  const std::string message(reinterpret_cast<const char*>(data),size);
  orbyx::translate(message,"it");orbyx::translate(message,"en");
 }catch(const std::exception&){}
 return 0;
}
