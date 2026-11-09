// SPDX-License-Identifier: GPL-3.0-only
// Curated six-JPEG demo. This adapter deliberately does not fabricate RDAS metadata.
#include "orbyx/engine.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <json/json.h>
#include <fstream>
#include <cmath>
#include <stdexcept>
namespace orbyx {
namespace {
void checkDemo(bool condition,const char *message) {
    if(!condition) throw std::runtime_error(message);
}
cv::Mat readDemoJpeg(const std::filesystem::path& source) {
    checkDemo(std::filesystem::is_regular_file(std::filesystem::symlink_status(source)),"Demo input must be a regular file");
    const auto size=std::filesystem::file_size(source);
    checkDemo(size>=4&&size<=1024*1024,"Demo JPEG size exceeds bounds");
    std::vector<unsigned char> bytes(size);
    std::ifstream stream(source,std::ios::binary);stream.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
    checkDemo(size_t(stream.gcount())==size&&stream.peek()==std::char_traits<char>::eof(),"Demo JPEG changed while reading");
    checkDemo(bytes[0]==255&&bytes[1]==216,"Demo input is not JPEG");
    bool dimensions=false;
    for(size_t offset=2;offset+3<bytes.size();) {
        checkDemo(bytes[offset++]==255,"Invalid JPEG marker");
        while(offset<bytes.size()&&bytes[offset]==255)++offset;
        if(offset>=bytes.size())throw std::runtime_error("Incomplete JPEG marker");
        auto marker=bytes.at(offset++);
        if(marker==0xda||marker==0xd9)break;
        if(marker==0x01||(marker>=0xd0&&marker<=0xd7))continue;
        checkDemo(offset+2<=bytes.size(),"Incomplete JPEG length");
        size_t length=(unsigned(bytes[offset])<<8)|bytes[offset+1];
        checkDemo(length>=2&&length<=bytes.size()-offset,"Invalid JPEG segment length");
        if(marker==0xc0||marker==0xc1||marker==0xc2) {
            checkDemo(length>=8,"Incomplete JPEG dimensions");
            int h=(unsigned(bytes[offset+3])<<8)|bytes[offset+4],w=(unsigned(bytes[offset+5])<<8)|bytes[offset+6];
            checkDemo(bytes[offset+2]==8&&w==714&&h==2048,"Demo requires bundled 714x2048 8-bit JPEGs");
            dimensions=true;break;
        }
        offset+=length;
    }
    checkDemo(dimensions,"JPEG dimensions not found");
    auto image=cv::imdecode(bytes,cv::IMREAD_GRAYSCALE|cv::IMREAD_IGNORE_ORIENTATION);
    checkDemo(!image.empty()&&image.cols==714&&image.rows==2048&&image.type()==CV_8UC1,"Cannot decode demo JPEG");
    return image;
}
void saveDemo(const std::filesystem::path& file,const cv::Mat& image) {
    checkDemo(cv::imwrite(file.string(),image),"Could not save demo image");
}
// Correlate adjacent edge profiles. These halves have no verified overlap, so
// never estimate a homography or fabricate intermediate pixels.
std::pair<int,double> seamShift(const cv::Mat& left,const cv::Mat& right) {
    double best=-2; int shift=0;
    for(int dy=-12;dy<=12;++dy) {
        std::vector<double> a,b;
        for(int y=140;y<1380;++y) {
            a.push_back(cv::mean(left(cv::Rect(left.cols-5,y,5,1)))[0]-cv::mean(left(cv::Rect(left.cols-5,y-1,5,1)))[0]);
            b.push_back(cv::mean(right(cv::Rect(0,y+dy,5,1)))[0]-cv::mean(right(cv::Rect(0,y+dy-1,5,1)))[0]);
        }
        double ma=cv::mean(a)[0],mb=cv::mean(b)[0],aa=0,bb=0,ab=0;
        for(size_t i=0;i<a.size();++i) {double x=a[i]-ma,z=b[i]-mb;aa+=x*x;bb+=z*z;ab+=x*z;}
        double score=aa>0&&bb>0?ab/std::sqrt(aa*bb):-1;
        if(score>best) {best=score;shift=dy;}
    }
    checkDemo(best>.5,"Demo halves have no reliable adjacent-edge alignment");
    return {shift,best};
}
}
void runImageDemo(const std::filesystem::path& input,const std::filesystem::path& output,const Log& log) {
    namespace fs=std::filesystem;
    // Output creation is exclusive. Never delete a destination owned by someone else.
    checkDemo(fs::create_directory(output),"Demo output already exists");
    try {
        fs::permissions(output,fs::perms::owner_all,fs::perm_options::replace);
        Json::Value report;report["schema_version"]=1;report["source"]="six user-supplied 714x2048 JPEG previews";
        report["limitations"]="JPEG 8-bit; no CBOR, absolute geometry or radiometric true colour validation; adjacent halves, no verified overlap";
        std::array<cv::Mat,2> natural,reference;
        for(int side=0;side<2;++side) {
            const std::string name=side==0?"sx":"dx";
            std::array<cv::Mat,3> images;
            std::array<fs::path,3> files;
            for(int c=0;c<3;++c) {
                const auto source=input/(name+"_MSUGS-VIS-"+std::to_string(c+1)+".jpg");
                checkDemo(!fs::is_symlink(fs::symlink_status(source)),"Demo image must not be a symlink");
                auto image=readDemoJpeg(source);
                images[c]=image(cv::Rect(0,80,714,1968)).clone(); // exclude calibration ramps
                files[c]=output/(name+"-ch"+std::to_string(c+1)+".png");
            }
            reference[side]=images[0].clone();
            cv::Mat ref;images[0].convertTo(ref,CV_32F);
            for(int c=1;c<3;++c) {
                cv::Mat target;images[c].convertTo(target,CV_32F);
                double response=0;auto shift=cv::phaseCorrelate(ref,target,cv::noArray(),&response);
                checkDemo(response>.2&&std::isfinite(shift.x)&&std::isfinite(shift.y),"Unreliable demo channel displacement");
                cv::Mat warp=(cv::Mat_<float>(2,3)<<1,0,float(shift.x),0,1,float(shift.y));
                const double score=cv::findTransformECC(ref,target,warp,cv::MOTION_AFFINE,
                    cv::TermCriteria(cv::TermCriteria::COUNT|cv::TermCriteria::EPS,100,1e-6),cv::noArray(),5);
                checkDemo(std::isfinite(score)&&score>.85&&cv::checkRange(warp),"Demo ECC registration failed");
                checkDemo(std::abs(warp.at<float>(0,0)-1)<.03&&std::abs(warp.at<float>(1,1)-1)<.03&&
                    std::abs(warp.at<float>(1,0))<.03&&std::abs(warp.at<float>(0,1))<.03&&
                    std::abs(warp.at<float>(0,2))<20&&std::abs(warp.at<float>(1,2))<500,"Unexpected demo geometry");
                cv::warpAffine(images[c],images[c],warp,images[0].size(),cv::INTER_LINEAR|cv::WARP_INVERSE_MAP);
                auto& entry=report["alignment"][name][std::to_string(c+1)];entry["ecc"]=score;entry["phase_response"]=response;
                for(int y=0;y<2;++y)for(int x=0;x<3;++x)entry["inverse_affine"].append(warp.at<float>(y,x));
                if(log)log(name+" CH"+std::to_string(c+1)+": ECC="+std::to_string(score));
            }
            for(int c=0;c<3;++c)saveDemo(files[c],images[c]);
            composeNaturalColor(files,output/(name+"-natural.png"));
            natural[side]=cv::imread((output/(name+"-natural.png")).string());
        }
        auto [shift,score]=seamShift(reference[0],reference[1]);
        report["seam"]["right_down_pixels"]=-shift;report["seam"]["edge_correlation"]=score;
        report["seam"]["horizontal_overlap_pixels"]=0;
        cv::Mat adjusted;
        cv::Mat seamWarp=(cv::Mat_<double>(2,3)<<1,0,0,0,1,-shift);
        cv::warpAffine(natural[1],adjusted,seamWarp,natural[1].size(),cv::INTER_LINEAR,cv::BORDER_CONSTANT,cv::Scalar(8,8,8));
        cv::Mat joined;cv::hconcat(natural[0],adjusted,joined);
        // Fixed framing of the complete disk in this documented sample, without rescaling.
        cv::Mat raw=joined(cv::Rect(0,45,1428,1428)).clone();
        saveDemo(output/"composite-raw.png",raw);
        cv::Mat display;raw.convertTo(display,CV_32F,1./180.,-8./180.);
        cv::max(display,0,display);cv::min(display,1,display);cv::pow(display,.8,display);display.convertTo(display,CV_8U,255);
        saveDemo(output/"composite.png",display);
        report["display"]["black"]=8;report["display"]["white"]=188;report["display"]["gamma_exponent"]=.8;
        report["display"]["crop_after_header"]=Json::arrayValue;
        for(int v:{0,45,1428,1428})report["display"]["crop_after_header"].append(v);
        Json::StreamWriterBuilder writer;std::ofstream stream(output/"demo-report.json");stream<<Json::writeString(writer,report);stream.close();
        checkDemo(!stream.fail(),"Could not save demo report");
        if(log)log("6 JPEG -> 1 composite.png (1428x1428), RGB321 / Natural Color; display contrast recorded");
    } catch(...) {fs::remove_all(output);throw;}
}
}
