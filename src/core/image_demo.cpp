// SPDX-License-Identifier: GPL-3.0-only
// Curated six-JPEG demo. This adapter deliberately does not fabricate RDAS metadata.
#include "orbyx/engine.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <json/json.h>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <array>
#include <stdexcept>
namespace orbyx {
namespace {
void checkDemo(bool condition,const char *message) {
    if(!condition) throw std::runtime_error(message);
}
cv::Mat readDemoImage(const std::filesystem::path& source) {
    checkDemo(std::filesystem::is_regular_file(std::filesystem::symlink_status(source)),"Demo input must be a regular file");
    const auto size=std::filesystem::file_size(source);
    checkDemo(size>=4&&size<=40*1024*1024,"Demo image size exceeds bounds");
    std::vector<unsigned char> bytes(size);
    std::ifstream stream(source,std::ios::binary);stream.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
    checkDemo(size_t(stream.gcount())==size&&stream.peek()==std::char_traits<char>::eof(),"Demo image changed while reading");
    const bool isJpeg=bytes[0]==255&&bytes[1]==216;
    const bool isPng=size>=33&&bytes[0]==0x89&&bytes[1]==0x50&&bytes[2]==0x4E&&bytes[3]==0x47&&
        bytes[4]==0x0D&&bytes[5]==0x0A&&bytes[6]==0x1A&&bytes[7]==0x0A;
    checkDemo(isJpeg||isPng,"Demo input must be a JPEG or PNG image");
    int w=0,h=0;
    if(isJpeg) {
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
                h=(unsigned(bytes[offset+3])<<8)|bytes[offset+4];w=(unsigned(bytes[offset+5])<<8)|bytes[offset+6];
                checkDemo(bytes[offset+2]==8,"Demo requires 8-bit JPEGs");
                dimensions=true;break;
            }
            offset+=length;
        }
        checkDemo(dimensions,"JPEG dimensions not found");
    } else {
        // PNG: validate the IHDR chunk and read the declared dimensions. Any bit
        // depth is accepted; IMREAD_GRAYSCALE normalises it to 8-bit for the demo.
        checkDemo(bytes[12]=='I'&&bytes[13]=='H'&&bytes[14]=='D'&&bytes[15]=='R',"Invalid PNG header");
        w=int((unsigned(bytes[16])<<24)|(unsigned(bytes[17])<<16)|(unsigned(bytes[18])<<8)|unsigned(bytes[19]));
        h=int((unsigned(bytes[20])<<24)|(unsigned(bytes[21])<<16)|(unsigned(bytes[22])<<8)|unsigned(bytes[23]));
    }
    checkDemo(w>0&&h>0&&w<=20000&&h<=20000&&static_cast<long long>(w)*h<=150000000,
        "Demo image resolution out of range (max 20000 per side, 150 Mpx)");
    auto image=cv::imdecode(bytes,cv::IMREAD_GRAYSCALE|cv::IMREAD_IGNORE_ORIENTATION);
    checkDemo(!image.empty()&&image.type()==CV_8UC1&&image.cols==w&&image.rows==h,"Cannot decode demo image");
    return image;
}
void saveDemo(const std::filesystem::path& file,const cv::Mat& image) {
    checkDemo(cv::imwrite(file.string(),image),"Could not save demo image");
}
// Correlate adjacent edge profiles. These halves have no verified overlap, so
// never estimate a homography or fabricate intermediate pixels.
std::pair<int,double> seamShift(const cv::Mat& left,const cv::Mat& right,bool bundled) {
    const int rows=std::min(left.rows,right.rows);
    // The bundled sample keeps the documented fixed band (140..1380, ±12) so its
    // measured seam stays identical; other sizes scale the same proportions.
    const int y0=bundled?140:static_cast<int>(std::lround(0.07*rows));
    const int y1=bundled?1380:static_cast<int>(std::lround(0.70*rows));
    const int dyMax=bundled?12:std::clamp(static_cast<int>(std::lround(0.02*rows)),4,64);
    const int lo=std::max(y0,dyMax+1),hi=std::min(y1,rows-dyMax);
    checkDemo(left.cols>=5&&right.cols>=5&&hi-lo>50,"Demo halves too small for seam estimation");
    double best=-2; int shift=0;
    for(int dy=-dyMax;dy<=dyMax;++dy) {
        std::vector<double> a,b;
        for(int y=lo;y<hi;++y) {
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
void runImageDemo(const std::filesystem::path& input,const std::filesystem::path& output,const Log& log,int exportScale) {
    namespace fs=std::filesystem;
    // Output creation is exclusive. Never delete a destination owned by someone else.
    checkDemo(fs::create_directory(output),"Demo output already exists");
    try {
        fs::permissions(output,fs::perms::owner_all,fs::perm_options::replace);
        Json::Value report;report["schema_version"]=1;
        report["limitations"]="JPEG 8-bit; no CBOR, absolute geometry or radiometric true colour validation; adjacent halves, no verified overlap";
        std::array<cv::Mat,2> natural,reference;
        cv::Size baseSize(-1,-1);bool bundled=false;int header=0;
        for(int side=0;side<2;++side) {
            const std::string name=side==0?"sx":"dx";
            std::array<cv::Mat,3> images;
            std::array<fs::path,3> files;
            for(int c=0;c<3;++c) {
                const std::string stem=name+"_MSUGS-VIS-"+std::to_string(c+1);
                // Accept either a lossless PNG (preferred for SatDump exports) or a JPEG.
                fs::path source=input/(stem+".jpg");
                if(!fs::is_regular_file(fs::symlink_status(source)))source=input/(stem+".png");
                checkDemo(!fs::is_symlink(fs::symlink_status(source)),"Demo image must not be a symlink");
                auto image=readDemoImage(source);
                if(baseSize.width<0) {
                    baseSize=image.size();
                    bundled=(baseSize==cv::Size(714,2048));
                    // Only the bundled 714x2048 previews carry an 80-row calibration
                    // ramp to exclude; user images of any other size are used whole.
                    header=bundled?80:0;
                }
                checkDemo(image.size()==baseSize,"The six demo JPEGs must share identical dimensions");
                images[c]=image(cv::Rect(0,header,baseSize.width,baseSize.height-header)).clone();
                files[c]=output/(name+"-ch"+std::to_string(c+1)+".png");
            }
            reference[side]=images[0].clone();
            // Estimate the per-channel registration on a bounded working image
            // (<=2048 px long side) to keep CPU and memory sane on large inputs;
            // the resulting transform is rescaled and applied at full resolution.
            // The bundled 1968-px halves (and any image already <=2048) use est=1,
            // so their estimation and byte output are unchanged.
            const double est=std::min(1.0,2048.0/std::max(images[0].cols,images[0].rows));
            auto reduced=[&](const cv::Mat& m){cv::Mat r=m;if(est<1.0)cv::resize(m,r,{},est,est,cv::INTER_AREA);cv::Mat f;r.convertTo(f,CV_32F);return f;};
            cv::Mat ref=reduced(images[0]);
            for(int c=1;c<3;++c) {
                cv::Mat target=reduced(images[c]);
                double response=0;auto shift=cv::phaseCorrelate(ref,target,cv::noArray(),&response);
                checkDemo(response>.2&&std::isfinite(shift.x)&&std::isfinite(shift.y),"Unreliable demo channel displacement");
                cv::Mat warp=(cv::Mat_<float>(2,3)<<1,0,float(shift.x),0,1,float(shift.y));
                const double score=cv::findTransformECC(ref,target,warp,cv::MOTION_AFFINE,
                    cv::TermCriteria(cv::TermCriteria::COUNT|cv::TermCriteria::EPS,100,1e-6),cv::noArray(),5);
                checkDemo(std::isfinite(score)&&score>.85&&cv::checkRange(warp),"Demo ECC registration failed");
                // Translations were measured in the reduced frame; rescale to full pixels.
                warp.at<float>(0,2)=float(warp.at<float>(0,2)/est);warp.at<float>(1,2)=float(warp.at<float>(1,2)/est);
                const double maxTx=bundled?20.0:0.1*baseSize.width;
                const double maxTy=bundled?500.0:0.3*(baseSize.height-header);
                checkDemo(std::abs(warp.at<float>(0,0)-1)<.03&&std::abs(warp.at<float>(1,1)-1)<.03&&
                    std::abs(warp.at<float>(1,0))<.03&&std::abs(warp.at<float>(0,1))<.03&&
                    std::abs(warp.at<float>(0,2))<maxTx&&std::abs(warp.at<float>(1,2))<maxTy,"Unexpected demo geometry");
                cv::warpAffine(images[c],images[c],warp,images[0].size(),cv::INTER_LINEAR|cv::WARP_INVERSE_MAP);
                auto& entry=report["alignment"][name][std::to_string(c+1)];entry["ecc"]=score;entry["phase_response"]=response;
                for(int y=0;y<2;++y)for(int x=0;x<3;++x)entry["inverse_affine"].append(warp.at<float>(y,x));
                if(log)log(name+" CH"+std::to_string(c+1)+": ECC="+std::to_string(score));
            }
            for(int c=0;c<3;++c)saveDemo(files[c],images[c]);
            composeNaturalColor(files,output/(name+"-natural.png"));
            natural[side]=cv::imread((output/(name+"-natural.png")).string());
        }
        // Estimate the seam on a bounded working image as well (bundled halves are
        // <=2048 px, so seamEst=1 and the measured seam is byte-identical); the
        // integer shift is then scaled back to full-resolution pixels.
        const double seamEst=std::min(1.0,2048.0/std::max({reference[0].rows,reference[0].cols,reference[1].rows,reference[1].cols}));
        cv::Mat seamL=reference[0],seamR=reference[1];
        if(seamEst<1.0){cv::resize(reference[0],seamL,{},seamEst,seamEst,cv::INTER_AREA);cv::resize(reference[1],seamR,{},seamEst,seamEst,cv::INTER_AREA);}
        auto [shiftEst,score]=seamShift(seamL,seamR,bundled);
        const int shift=int(std::lround(shiftEst/seamEst));
        report["seam"]["right_down_pixels"]=-shift;report["seam"]["edge_correlation"]=score;
        report["seam"]["horizontal_overlap_pixels"]=0;
        cv::Mat adjusted;
        cv::Mat seamWarp=(cv::Mat_<double>(2,3)<<1,0,0,0,1,-shift);
        cv::warpAffine(natural[1],adjusted,seamWarp,natural[1].size(),cv::INTER_LINEAR,cv::BORDER_CONSTANT,cv::Scalar(8,8,8));
        cv::Mat joined;cv::hconcat(natural[0],adjusted,joined);
        // The bundled sample is framed to the documented disk crop without rescaling;
        // user images of any other size keep the full joined frame (seam may need manual touch-up).
        cv::Mat raw=bundled?joined(cv::Rect(0,45,1428,1428)).clone():joined.clone();
        // MSU-GS VIS previews are acquired South-up; present the finished composite
        // North-up. Rotation is applied to the whole composed frame after cropping.
        cv::rotate(raw,raw,cv::ROTATE_180);
        // Optional export enlargement (1..4, interpolated: more pixels, not more
        // detail). Clamp the factor so the enlarged output stays bounded in memory.
        int scale=std::clamp(exportScale,1,4);
        while(scale>1&&(static_cast<long long>(raw.cols)*scale>20000||static_cast<long long>(raw.rows)*scale>20000||
              static_cast<long long>(raw.cols)*raw.rows*scale*scale>200000000))--scale;
        if(scale>1)cv::resize(raw,raw,cv::Size(raw.cols*scale,raw.rows*scale),0,0,cv::INTER_CUBIC);
        report["display"]["export_scale"]=scale;
        saveDemo(output/"composite-raw.png",raw);
        cv::Mat display;raw.convertTo(display,CV_32F,1./180.,-8./180.);
        cv::max(display,0,display);cv::min(display,1,display);cv::pow(display,.8,display);display.convertTo(display,CV_8U,255);
        saveDemo(output/"composite.png",display);
        report["display"]["black"]=8;report["display"]["white"]=188;report["display"]["gamma_exponent"]=.8;
        report["display"]["rotated_180"]=true;
        report["display"]["crop_after_header"]=Json::arrayValue;
        const std::array<int,4> crop=bundled?std::array<int,4>{0,45,1428,1428}:std::array<int,4>{0,0,raw.cols,raw.rows};
        for(int v:crop)report["display"]["crop_after_header"].append(v);
        report["geometry"]["input_width"]=baseSize.width;report["geometry"]["input_height"]=baseSize.height;
        report["geometry"]["bundled_sample"]=bundled;report["geometry"]["output_width"]=raw.cols;report["geometry"]["output_height"]=raw.rows;
        report["source"]=bundled?"six bundled 714x2048 JPEG previews":"six user-supplied JPEG previews";
        Json::StreamWriterBuilder writer;std::ofstream stream(output/"demo-report.json");stream<<Json::writeString(writer,report);stream.close();
        checkDemo(!stream.fail(),"Could not save demo report");
        if(log)log("6 JPEG -> 1 composite.png ("+std::to_string(raw.cols)+"x"+std::to_string(raw.rows)+"), RGB321 / Natural Color; display contrast recorded");
    } catch(...) {fs::remove_all(output);throw;}
}
}
