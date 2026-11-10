// ORBYX — GPL-3.0. Synthetic regression fixtures, no satellite imagery required.
#include "orbyx/engine.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <json/json.h>
#include <filesystem>
#include <array>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <limits>
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
 const auto fullLayout=orbyx::editableMergeLayout(profile,"L2",{output/"MSUGS_VIS-1_Aligned/MSUGS-VIS-1.png",output/"MSUGS_VIS-2_Aligned/MSUGS-VIS-1.png"});
 check(!fullLayout.profileRoi&&fullLayout.regions[0].width==64&&fullLayout.regions[1].height==16,"Missing ROI must expose full editable products");
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
 check(!fs::exists(options.output/"MSUGS_VIS-1_Aligned"),"Default legacy ROI output unexpectedly gained editable products");
 options.editableParts=true;options.output=root/(name+"-merged-editable");orbyx::run(options);
 const auto editableMerged=cv::imread((options.output/"MSUGS_VIS_Aligned/MSUGS-VIS-2.png").string(),cv::IMREAD_UNCHANGED);
 check(cv::norm(merged,editableMerged,cv::NORM_INF)==0,"Editable opt-in changed historical ROI pixels");
 check(fs::is_regular_file(options.output/"MSUGS_VIS-1_Aligned/MSUGS-VIS-1.png")&&fs::is_regular_file(options.output/"MSUGS_VIS-2_Aligned/MSUGS-VIS-1.png"),"Editable aligned VIS products missing");
 const auto roiLayout=orbyx::editableMergeLayout(profile,"L2",{options.output/"MSUGS_VIS-1_Aligned/MSUGS-VIS-1.png",options.output/"MSUGS_VIS-2_Aligned/MSUGS-VIS-1.png"});
 check(roiLayout.profileRoi&&roiLayout.regions[0].x==1&&roiLayout.regions[0].width==20&&roiLayout.regions[1].height==10,"Historical ROI was not preserved as editable layout");
 config["L2VIS1"]["ROI"]["width"]=1000;writeJson(profile,config);options.output=root/(name+"-badroi");
 fails([&]{orbyx::run(options);});check(!fs::exists(options.output),"Invalid ROI leaves no destination");
 fails([&]{orbyx::editableMergeLayout(profile,"L2",{root/(name+"-merged-editable/MSUGS_VIS-1_Aligned/MSUGS-VIS-1.png"),root/(name+"-merged-editable/MSUGS_VIS-2_Aligned/MSUGS-VIS-1.png")});});
}
void selectedProducts(const fs::path& root) {
 const auto input=root/"selected-in",output=root/"selected-out",profile=root/"selected.json";fixture(input,CV_16U);
 fs::copy(input/"MSUGS_VIS1",input/"MSUGS_VIS3",fs::copy_options::recursive);
 const auto aligned=input/"MSUGS_VIS-4_Aligned";fs::create_directories(aligned);
 for(int channel=1;channel<=3;++channel)cv::imwrite((aligned/("MSUGS-VIS-"+std::to_string(channel)+".png")).string(),cv::Mat(27,31,CV_16U,cv::Scalar(channel*4000)));
 auto config=identity();config["L2VIS3"]=config["L2VIS1"];writeJson(profile,config);
 orbyx::RunOptions options{"L2",input,output,profile,true,false};options.products={"MSUGS_VIS3","MSUGS_VIS-4_Aligned"};
 orbyx::run(options);
 check(fs::is_regular_file(output/"MSUGS_VIS-3_Aligned/MSUGS-VIS-1.png")&&fs::is_regular_file(output/"MSUGS_VIS-4_Aligned/MSUGS-VIS-3.png"),"Selected raw/aligned products were not written");
 check(cv::imread((output/"MSUGS_VIS-3_Aligned/MSUGS-VIS-2.png").string(),cv::IMREAD_UNCHANGED).rows==16,"Selected raw product was not aligned");
 check(cv::imread((output/"MSUGS_VIS-4_Aligned/MSUGS-VIS-2.png").string(),cv::IMREAD_UNCHANGED).rows==27,"Already aligned selection was transformed again");
 check(read(output/"MSUGS_VIS-3_Aligned/product.cbor")=="synthetic-cbor-vis1","Selected product lost CBOR provenance");
 check(!fs::exists(output/"MSUGS_VIS-4_Aligned/product.cbor"),"Missing source CBOR was invented");
 options.output=root/"selected-duplicate";options.products={"MSUGS_VIS3","MSUGS_VIS-3_Aligned"};fails([&]{orbyx::run(options);});check(!fs::exists(options.output),"Duplicate selected id left output");
 options.output=root/"selected-invalid";options.products={"../MSUGS_VIS3"};fails([&]{orbyx::run(options);});check(!fs::exists(options.output),"Invalid selected path left output");
 options.output=root/"selected-calibrate";options.products={"MSUGS_VIS3"};options.calibrate=true;fails([&]{orbyx::run(options);});check(!fs::exists(options.output),"Selection in calibrate mode left output");
}
// Independent double-precision scalar RGB/HSV oracle, without cvtColor.
cv::Vec3d naturalOracle(cv::Vec3d bgr,double maximum) {
 const double r=bgr[2]/maximum,g=bgr[1]/maximum,b=bgr[0]/maximum;
 const double high=std::max({r,g,b}),low=std::min({r,g,b}),delta=high-low;
 double h=0;
 if(delta>0) {
  if(high==r)h=60*std::fmod((g-b)/delta+6,6);
  else if(high==g)h=60*((b-r)/delta+2);
  else h=60*((r-g)/delta+4);
 }
 if(h>=340 || h<=40)h+=60;
 else if(h<=100)h-=6;
 h=std::fmod(h,360);
 const double saturation=high==0?0:delta/high,c=high*saturation;
 const double x=c*(1-std::abs(std::fmod(h/60,2)-1)),m=high-c;
 cv::Vec3d rgb;
 if(h<60)rgb={c,x,0};else if(h<120)rgb={x,c,0};else if(h<180)rgb={0,c,x};
 else if(h<240)rgb={0,x,c};else if(h<300)rgb={x,0,c};else rgb={c,0,x};
 return cv::Vec3d(0.6*bgr[0]+0.4*(rgb[2]+m)*maximum,
                   0.6*bgr[1]+0.4*(rgb[1]+m)*maximum,
                   0.6*bgr[2]+0.4*(rgb[0]+m)*maximum);
}
void naturalColor(const fs::path& root,int type,const std::string& name) {
 const double maximum=type==CV_16U?65535:255;
 // Red, yellow, blue, green, magenta, grey, black, white, and interior hues.
 const std::array<cv::Vec3d,12> samples{{{0,0,1},{0,1,1},{1,0,0},{0,1,0},{1,0,1},
   {.37,.37,.37},{0,0,0},{1,1,1},{.11,.21,.93},{.04,.81,.97},{.2,.7,.4},{.9,.2,.3}}};
 std::array<cv::Mat,3> mats;
 std::array<fs::path,3> paths;
 std::array<std::string,3> originals;
 for(int channel=0;channel<3;++channel) {
  mats[channel].create(261,37,type);
  for(int y=0;y<261;++y)for(int x=0;x<37;++x) {
   const int value=cvRound(samples[(y*37+x)%samples.size()][channel]*maximum);
   if(type==CV_16U)mats[channel].at<unsigned short>(y,x)=static_cast<unsigned short>(value);
   else mats[channel].at<unsigned char>(y,x)=static_cast<unsigned char>(value);
  }
  paths[channel]=root/(name+"-ch"+std::to_string(channel)+".png");
  cv::imwrite(paths[channel].string(),mats[channel]);originals[channel]=read(paths[channel]);
 }
 const auto output=root/(name+"-natural.png");
 orbyx::composeNaturalColor(paths,output);
 const auto actual=cv::imread(output.string(),cv::IMREAD_UNCHANGED);
 check(actual.size()==mats[0].size()&&actual.type()==CV_MAKETYPE(type,3),"Natural Color dimensions and depth");
 for(int y=0;y<actual.rows;++y)for(int x=0;x<actual.cols;++x) {
  cv::Vec3d original,result;
  for(int c=0;c<3;++c) {
   original[c]=type==CV_16U?mats[c].at<unsigned short>(y,x):mats[c].at<unsigned char>(y,x);
   result[c]=type==CV_16U?actual.at<cv::Vec<unsigned short,3>>(y,x)[c]:actual.at<cv::Vec3b>(y,x)[c];
  }
  check(cv::norm(result-naturalOracle(original,maximum))<=2.0,"Natural Color scalar RGB321/HSV oracle, including tile boundaries");
 }
 for(int c=0;c<3;++c)check(read(paths[c])==originals[c],"Natural Color inputs unchanged");
 const auto before=read(output);fails([&]{orbyx::composeNaturalColor(paths,output);});
 check(read(output)==before,"Natural Color must not overwrite output");
 fails([&]{orbyx::composeNaturalColor(paths,root/(name+".jpg"));});
 auto mismatch=paths; mismatch[2]=root/(name+"-mismatch.png");
 cv::imwrite(mismatch[2].string(),cv::Mat(2,2,type,cv::Scalar(1)));
 fails([&]{orbyx::composeNaturalColor(mismatch,root/(name+"-invalid.png"));});
 check(!fs::exists(root/(name+"-invalid.png")),"Invalid colour input leaves no destination");
 if(type==CV_16U) {
  // Low bits must survive: a neutral 10001 cannot be represented by 8-bit scaling.
  for(int c=0;c<3;++c)cv::imwrite(paths[c].string(),cv::Mat(1,2,CV_16U,cv::Scalar(10001)));
  const auto low=root/(name+"-low-bits.png");orbyx::composeNaturalColor(paths,low);
  check(cv::imread(low.string(),cv::IMREAD_UNCHANGED).at<cv::Vec<unsigned short,3>>(0,0)==cv::Vec<unsigned short,3>(10001,10001,10001),"Natural Color preserves neutral 16-bit low bits");
 }
}
void scaledCalibration(const fs::path& root) {
 const auto input=root/"scaled-ecc", output=root/"scaled-ecc-output", profile=root/"scaled-ecc.json";
 cv::Mat reference=cv::Mat::zeros(3919,253,CV_16U), texture(240,193,CV_16U);
 cv::RNG rng(481);rng.fill(texture,cv::RNG::UNIFORM,4000,60000);
 cv::GaussianBlur(texture,texture,cv::Size(9,9),2.0);
 texture.copyTo(reference(cv::Rect(30,20,193,240)));
 for(int v=1;v<=2;++v) {
  const auto dir=input/("MSUGS_VIS"+std::to_string(v));fs::create_directories(dir);
  for(int channel=1;channel<=3;++channel) {
   cv::Mat image;
   if(channel==1)image=reference;
   else {
    const int offset=channel==2?3606:1804;
    const cv::Mat matrix=(cv::Mat_<float>(2,3)<<1.003,0.002,3.4,-0.001,1.001,offset+0.6);
    cv::warpAffine(reference,image,matrix,reference.size());
   }
   check(cv::imwrite((dir/("MSUGS-VIS-"+std::to_string(channel)+".png")).string(),image),"scaled affine fixture");
  }
  std::ofstream(dir/"product.cbor")<<"synthetic-scaled-affine";
 }
 writeJson(profile,Json::Value(Json::objectValue));
 orbyx::RunOptions options{"L4",input,output,profile,true,true,"en",0.5};
 orbyx::run(options);
 Json::Value calibrated;std::ifstream(output/"calibration.json")>>calibrated;
 for(const char* vis:{"L4VIS1","L4VIS2"}) for(const char* channel:{"CH2","CH3"}) {
  const auto& m=calibrated[vis][channel];const int offset=std::string(channel)=="CH2"?3606:1804;
  for(const cv::Point2d point:{cv::Point2d(40,30),cv::Point2d(210,245)}) {
   const double dx=m["a"].asDouble()*point.x+m["b"].asDouble()*point.y+m["tx"].asDouble()-(1.003*point.x+0.002*point.y+3.4);
   const double dy=m["c"].asDouble()*point.x+m["d"].asDouble()*point.y+m["ty"].asDouble()-(-0.001*point.x+1.001*point.y+offset+0.6);
   check(std::hypot(dx,dy)<0.75,"Scaled ECC must recover known affine in native pixel coordinates");
  }
 }
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
void profiles(const fs::path& root) {
 const auto profile=root/"dup-src.json";writeJson(profile,identity());
 const auto out=root/"dup-out.json";
 orbyx::duplicateProfile(profile,"L2","L4",out);
 Json::Value result;std::ifstream(out)>>result;
 check(result["L2VIS1"].isObject()&&result["L2VIS2"].isObject(),"Original entries preserved");
 for(const char* vis:{"VIS1","VIS2"}) {
  const std::string from=std::string("L2")+vis,to=std::string("L4")+vis;
  check(result[to].isObject(),"Duplicated satellite present");
  for(const char* ch:{"CH2","CH3"})
   check(result[to][ch]["ty"].asDouble()==result[from][ch]["ty"].asDouble(),"Duplicated satellite matches source");
 }
 Json::Value original;std::ifstream(profile)>>original;
 check(!original.isMember("L4VIS1"),"Source profile unchanged");
 // Existing destinations, missing source satellites and invalid ids are rejected.
 fails([&]{orbyx::duplicateProfile(profile,"L2","L5",out);});
 fails([&]{orbyx::duplicateProfile(profile,"M9","L4",root/"dup-missing.json");});
 check(!fs::exists(root/"dup-missing.json"),"No profile written for missing source");
 fails([&]{orbyx::duplicateProfile(profile,"l2","L4",root/"dup-bad.json");});
 fails([&]{orbyx::duplicateProfile(profile,"L2","L2",root/"dup-same.json");});
 check(!fs::exists(root/"dup-bad.json")&&!fs::exists(root/"dup-same.json"),"No profile written on rejection");
}
void compose(const fs::path& root) {
 for(int type:{CV_8U,CV_16U}) {
  const auto dir=root/("rgb-"+std::to_string(type));fs::create_directories(dir);
  std::array<fs::path,3> paths;const std::array<int,3> base{{10,20,30}};
  for(int i=0;i<3;++i) {
   cv::Mat ch(16,24,type,cv::Scalar(type==CV_8U?base[i]:base[i]*257));
   paths[i]=dir/("c"+std::to_string(i)+".png");
   check(cv::imwrite(paths[i].string(),ch),"compose channel write");
  }
  const auto out=dir/"rgb.png";orbyx::composeRgb(paths,out);
  const auto rgb=cv::imread(out.string(),cv::IMREAD_UNCHANGED);
  check(rgb.channels()==3&&rgb.depth()==type&&rgb.cols==24&&rgb.rows==16,"RGB shape and depth preserved");
  // OpenCV reads BGR: value order is [B=CH3, G=CH2, R=CH1].
  if(type==CV_8U){const auto px=rgb.at<cv::Vec3b>(0,0);check(px[2]==10&&px[1]==20&&px[0]==30,"RGB channel mapping 8-bit");}
  else {const auto px=rgb.at<cv::Vec3w>(0,0);check(px[2]==10*257&&px[1]==20*257&&px[0]==30*257,"RGB channel mapping 16-bit");}
  check(cv::imread(paths[0].string(),cv::IMREAD_UNCHANGED).channels()==1,"compose inputs unchanged");
  fails([&]{orbyx::composeRgb(paths,out);}); // destination must not be overwritten
  // M2-11 single-channel preset: the chosen channel as neutral gray, depth kept.
  for(int index=0;index<3;++index){
   const auto single=dir/("single-"+std::to_string(index)+".png");orbyx::composeSingleChannel(paths,single,index);
   const auto g=cv::imread(single.string(),cv::IMREAD_UNCHANGED);
   check(g.channels()==3&&g.depth()==type,"Single channel shape/depth preserved");
   const int expect=type==CV_8U?base[size_t(index)]:base[size_t(index)]*257;
   if(type==CV_8U){const auto px=g.at<cv::Vec3b>(0,0);check(px[0]==expect&&px[1]==expect&&px[2]==expect,"Single channel is neutral gray 8-bit");}
   else{const auto px=g.at<cv::Vec3w>(0,0);check(px[0]==expect&&px[1]==expect&&px[2]==expect,"Single channel is neutral gray 16-bit");}
  }
  check(cv::imread(paths[1].string(),cv::IMREAD_UNCHANGED).channels()==1,"Single channel inputs unchanged");
  fails([&]{orbyx::composeSingleChannel(paths,dir/"bad.png",3);}); // invalid index
  // M2-11 colour order: the three channels must share geometry (same size) before
  // colour composition, so R/G/B never get different geometry (no coloured fringe).
  cv::imwrite(paths[0].string(),cv::Mat(18,24,type,cv::Scalar(5)));
  fails([&]{orbyx::composeRgb(paths,dir/"size-mismatch.png");});
  check(!fs::exists(dir/"size-mismatch.png"),"No RGB product on channel size mismatch");
  cv::imwrite(paths[0].string(),cv::Mat(16,24,type==CV_8U?CV_16U:CV_8U,cv::Scalar(5)));
  fails([&]{orbyx::composeRgb(paths,dir/"mismatch.png");});
  check(!fs::exists(dir/"mismatch.png"),"No RGB product on depth mismatch");
 }
}
int main(int argc,char** argv) {
 fs::path root;
 if(argc!=1) {
  try {
   check(argc==3&&std::string(argv[1])=="--fixture","Usage: orbyx_tests [--fixture NEW_DIRECTORY]");
   const fs::path destination=fs::absolute(argv[2]);
   check(!fs::exists(fs::symlink_status(destination)),"Fixture destination already exists");
   fs::create_directory(destination);
   fixture(destination/"input",CV_8U);
   writeJson(destination/"profile.json",identity());
   std::cout<<"Synthetic fixture: "<<destination<<'\n';
   return 0;
  }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
 }
 try {
  root=fs::temp_directory_path()/("orbyx-tests-"+std::to_string(std::random_device{}()));
  fs::create_directory(root);
  separate(root,CV_8U,"8bit");separate(root,CV_16U,"16bit");selectedProducts(root);calibration(root);scaledCalibration(root);profiles(root);compose(root);naturalColor(root,CV_8U,"nc8");naturalColor(root,CV_16U,"nc16");
  auto options=orbyx::RunOptions{"../bad",root,root/"bad",root/"8bit.json",true,false};
  fails([&]{orbyx::run(options);});
  auto config=identity();writeJson(root/"valid.json",config);
  options={"L2",root/"16bit-in",root/"invalid-scale",root/"valid.json",true,true};
  for(const double scale:{0.0,-0.1,1.1,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}) {
    options.calibrationScale=scale;
    fails([&]{orbyx::run(options);});
    check(!fs::exists(options.output),"Invalid calibration scale must leave no output");
  }
  options.calibrationScale=0.5; options.calibrate=false;
  fails([&]{orbyx::run(options);});
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
