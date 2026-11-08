// ORBYX — GPL-3.0. Synthetic regression fixtures, no satellite imagery required.
#include "orbyx/engine.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <json/json.h>
#include <filesystem>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
namespace fs=std::filesystem;
void check(bool condition,const char* message) {if(!condition)throw std::runtime_error(message);}
template<class F> void fails(F f) {bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"Expected rejection");}
void writeJson(const fs::path& file,const Json::Value& value) {std::ofstream(file)<<value;}
Json::Value identity() {
 Json::Value root;
 for(const auto* vis:{"L2VIS1","L2VIS2"}) for(const auto* channel:{"CH2","CH3"}) {
  auto& m=root[vis][channel];
  m["a"]=1;m["b"]=0;m["c"]=0;m["d"]=1;m["tx"]=0;m["ty"]=channel==std::string("CH2")?3606:1804;
 }
 return root;
}
void fixture(const fs::path& input,int type) {
 for(int v=1;v<=2;++v) {
  const auto dir=input/("MSUGS_VIS"+std::to_string(v));fs::create_directories(dir);
  for(int c=1;c<=3;++c) {
   cv::Mat image(3622,64,type,cv::Scalar(0));
   const int offset=c==2?3606:c==3?1804:0;
   for(int y=0;y<16;++y)for(int x=0;x<64;++x) {
    const int value=10+v*30+c*10+(x+y)%30;
    if(type==CV_8U)image.at<unsigned char>(y+offset,x)=static_cast<unsigned char>(value);
    else image.at<unsigned short>(y+offset,x)=static_cast<unsigned short>(value*257);
   }
   check(cv::imwrite((dir/("MSUGS-VIS-"+std::to_string(c)+".png")).string(),image),"fixture write");
  }
  std::ofstream(dir/"product.cbor",std::ios::binary)<<"synthetic-cbor-vis"<<v;
 }
}
std::string read(const fs::path& file) {std::ifstream s(file);return std::string(std::istreambuf_iterator<char>(s),{});}
void separate(const fs::path& root,int type,const std::string& name) {
 const auto input=root/(name+"-in"),output=root/(name+"-out"),profile=root/(name+".json");
 fixture(input,type);auto config=identity();writeJson(profile,config);
 orbyx::RunOptions options{"L2",input,output,profile,true,false};
 orbyx::run(options);
 const auto result=cv::imread((output/"MSUGS_VIS-1_Aligned/MSUGS-VIS-2.png").string(),cv::IMREAD_UNCHANGED);
 const auto original=cv::imread((input/"MSUGS_VIS1/MSUGS-VIS-2.png").string(),cv::IMREAD_UNCHANGED);
 check(result.rows==16&&result.cols==64&&result.type()==type,"aligned dimensions/depth");
 check(cv::norm(result,original(cv::Rect(0,3606,64,16)),cv::NORM_INF)==0,"Must save transformed channel, not original");
 check(read(output/"MSUGS_VIS-2_Aligned/product.cbor")=="synthetic-cbor-vis2","VIS2 CBOR provenance");
 check(cv::imread((input/"MSUGS_VIS1/MSUGS-VIS-2.png").string(),cv::IMREAD_UNCHANGED).rows==3622,"Input unchanged");
 fails([&]{orbyx::run(options);});
 const auto inspection=orbyx::inspect(input/"MSUGS_VIS1/MSUGS-VIS-1.png");
 check(inspection.width==64&&inspection.height==3622&&inspection.depth==(type==CV_8U?8:16),"inspect");
 options.output=root/(name+"-invalid");
 config["L2VIS1"]["CH2"]["a"]="bad";writeJson(profile,config);
 fails([&]{orbyx::run(options);});check(!fs::exists(options.output),"No partial destination on failure");
 config=identity();for(auto vis:{"L2VIS1","L2VIS2"}) {
  auto& roi=config[vis]["ROI"];roi["x"]=1;roi["y"]=0;roi["width"]=20;roi["height"]=vis==std::string("L2VIS1")?16:10;
 }
 writeJson(profile,config);options.output=root/(name+"-merged");orbyx::run(options);
 const auto merged=cv::imread((options.output/"MSUGS_VIS_Aligned/MSUGS-VIS-2.png").string(),cv::IMREAD_UNCHANGED);
 check(merged.cols==40&&merged.rows==16,"Merged canvas uses actual ROI dimensions");
 check(cv::countNonZero(merged(cv::Rect(20,10,20,6)))==0,"Uncovered merged pixels initialized to zero");
 config["L2VIS1"]["ROI"]["width"]=1000;writeJson(profile,config);options.output=root/(name+"-badroi");
 fails([&]{orbyx::run(options);});check(!fs::exists(options.output),"Invalid ROI leaves no destination");
}
void calibration(const fs::path& root) {
 const auto input=root/"ecc-input",output=root/"ecc-calibration";
 cv::Mat noise(16,64,CV_8U);cv::RNG rng(173);rng.fill(noise,cv::RNG::UNIFORM,10,245);
 cv::GaussianBlur(noise,noise,cv::Size(3,3),0.6);
 for(int v=1;v<=2;++v) {
  const auto directory=input/("MSUGS_VIS"+std::to_string(v));fs::create_directories(directory);
  for(int c=1;c<=3;++c) {
   cv::Mat image=cv::Mat::zeros(3622,64,CV_8U);
   const int offset=c==2?3606:c==3?1804:0;
   noise.copyTo(image(cv::Rect(0,offset,64,16)));
   check(cv::imwrite((directory/("MSUGS-VIS-"+std::to_string(c)+".png")).string(),image),"ECC fixture write");
  }
  std::ofstream(directory/"product.cbor")<<"ecc-synthetic-"<<v;
 }
 const auto profile=root/"ecc-initial.json";writeJson(profile,Json::Value(Json::objectValue));
 orbyx::RunOptions options{"L4",input,output,profile,true,true};orbyx::run(options);
 Json::Value calibrated;std::ifstream(output/"calibration.json")>>calibrated;
 for(const auto* vis:{"L4VIS1","L4VIS2"})for(const auto* channel:{"CH2","CH3"}) {
  const auto& matrix=calibrated[vis][channel];const int offset=channel==std::string("CH2")?3606:1804;
  check(std::abs(matrix["a"].asDouble()-1)<0.001 && std::abs(matrix["d"].asDouble()-1)<0.001,"ECC identity linear coefficients");
  check(std::abs(matrix["tx"].asDouble())<0.1 && std::abs(matrix["ty"].asDouble()-offset)<0.1,"ECC recovered known channel translation");
 }
 options.calibrate=false;options.config=output/"calibration.json";options.output=root/"ecc-generated";
 orbyx::run(options);
 const auto generated=cv::imread((options.output/"MSUGS_VIS-1_Aligned/MSUGS-VIS-2.png").string(),cv::IMREAD_UNCHANGED);
 check(cv::norm(generated,noise,cv::NORM_INF)<=1,"Generated output uses freshly calibrated profile");
 // Existing output directories and symlinks must never be replaced.
 const auto existing=root/"existing";fs::create_directory(existing);std::ofstream(existing/"sentinel")<<"keep";
 options.output=existing;fails([&]{orbyx::run(options);});check(read(existing/"sentinel")=="keep","Existing output retained");
 fs::create_directory_symlink(existing,root/"output-link");options.output=root/"output-link";
 fails([&]{orbyx::run(options);});check(fs::is_symlink(options.output)&&read(existing/"sentinel")=="keep","Output symlink retained");
 // A competing producer creates the destination after processing but before publication.
 // It must retain ownership; no overwrite and no partial products may escape staging.
 options.output=root/"competing-output";
 bool competingCreated=false;
 fails([&]{orbyx::run(options,[&](const std::string& message) {
  if(message.rfind("CBOR copied unchanged",0)==0) {
   fs::create_directory(options.output);std::ofstream(options.output/"sentinel")<<"competitor";
   competingCreated=true;
  }
 });});
 check(competingCreated&&read(options.output/"sentinel")=="competitor","Concurrent destination retained");
 check(!fs::exists(options.output/"MSUGS_VIS-1_Aligned"),"Concurrent destination contains no partial products");
 // Mismatched dimensions must be diagnosed before an output becomes visible.
 cv::imwrite((input/"MSUGS_VIS2/MSUGS-VIS-2.png").string(),cv::Mat::zeros(3623,64,CV_8U));
 options.output=root/"mismatch";fails([&]{orbyx::run(options);});check(!fs::exists(options.output),"Dimension mismatch rollback");
 fs::remove(input/"MSUGS_VIS2/MSUGS-VIS-2.png");
 fs::create_symlink(input/"MSUGS_VIS1/MSUGS-VIS-2.png",input/"MSUGS_VIS2/MSUGS-VIS-2.png");
 options.output=root/"input-link";fails([&]{orbyx::run(options);});check(!fs::exists(options.output),"Input symlink rejected");
}
int main() {
 fs::path root;
 try {
  root=fs::temp_directory_path()/("orbyx-tests-"+std::to_string(std::random_device{}()));
  fs::create_directory(root);
  separate(root,CV_8U,"8bit");separate(root,CV_16U,"16bit");calibration(root);
  auto options=orbyx::RunOptions{"../bad",root,root/"bad",root/"8bit.json",true,false};
  fails([&]{orbyx::run(options);});
  auto config=identity();writeJson(root/"valid.json",config);
  options={"L2",root/"8bit-in",root/"corrupt",root/"valid.json",true,false};
  std::ofstream(root/"8bit-in/MSUGS_VIS2/MSUGS-VIS-3.png")<<"not a png";
  fails([&]{orbyx::run(options);});check(!fs::exists(options.output),"Corrupt input transaction rollback");
  // Flat channels force ECC failure; worker errors must propagate, never terminate.
  fixture(root/"cal-in",CV_8U);
  for(int v=1;v<=2;++v)for(int c=1;c<=3;++c)
    cv::imwrite((root/"cal-in"/("MSUGS_VIS"+std::to_string(v))/("MSUGS-VIS-"+std::to_string(c)+".png")).string(),cv::Mat::zeros(3622,64,CV_8U));
  options={"L4",root/"cal-in",root/"cal-out",root/"valid.json",true,true};
  fails([&]{orbyx::run(options);});check(!fs::exists(options.output),"ECC failure leaves no destination");
  // Encoded-size limits are checked before snapshot allocation/JSON parsing.
  std::ofstream(root/"oversized.png",std::ios::binary).put('x');
  fs::resize_file(root/"oversized.png",1024ULL*1024*1024+1);
  fails([&]{orbyx::inspect(root/"oversized.png");});
  std::ofstream(root/"oversized.json",std::ios::binary).put('{');
  fs::resize_file(root/"oversized.json",1024*1024+1);
  options={"L2",root/"16bit-in",root/"oversized-config-out",root/"oversized.json",true,false};
  fails([&]{orbyx::run(options);});
  check(!fs::exists(options.output),"Oversized profile leaves no destination");
  std::ofstream(root/"duplicate.json")<<"{\"L2VIS1\":{},\"L2VIS1\":{}}";
  options.config=root/"duplicate.json";
  fails([&]{orbyx::run(options);});
  check(!fs::exists(options.output),"Duplicate JSON keys rejected");
  std::ofstream(root/"truncated.png",std::ios::binary)<<"\x89PNG\r\n\x1a\n";
  fails([&]{orbyx::inspect(root/"truncated.png");});
  // A huge IHDR must be rejected before the PNG decoder allocates memory.
  std::array<unsigned char,33> huge{137,80,78,71,13,10,26,10,0,0,0,13,'I','H','D','R',0x7f,0xff,0xff,0xff,0x7f,0xff,0xff,0xff,16,0,0,0,0,0,0,0,0};
  std::ofstream bomb(root/"huge.png",std::ios::binary);bomb.write(reinterpret_cast<char*>(huge.data()),huge.size());bomb.close();
  fails([&]{orbyx::inspect(root/"huge.png");});
  check(orbyx::translate("Loading L2VIS1","it")=="Caricamento L2VIS1","Italian log translation");
  for(const auto& item:fs::directory_iterator(root))check(item.path().filename().string().rfind(".orbyx-",0)!=0,"Staging cleaned");
  fs::remove_all(root);std::cout<<"Core synthetic regressions passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';if(!root.empty())fs::remove_all(root);return 1;}
}
