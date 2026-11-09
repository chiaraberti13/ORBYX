// SPDX-License-Identifier: GPL-3.0-only
#include "orbyx/compositor.hpp"
#include <opencv2/imgproc.hpp>
#include <json/json.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>
namespace orbyx {
namespace {
void check(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
void validate(const MosaicLayer& l) {
    for(double v:{l.x,l.y,l.scale,l.angle,l.opacity,l.feather,l.black,l.white,l.gamma,l.exposure,l.saturation,l.red,l.green,l.blue})
        check(std::isfinite(v),"Non-finite mosaic parameter");
    check(std::abs(l.x)<=1000000&&std::abs(l.y)<=1000000&&l.scale>=.01&&l.scale<=100&&std::abs(l.angle)<=360,
          "Mosaic transform exceeds limits");
    check(l.opacity>=0&&l.opacity<=1&&l.feather>=0&&l.feather<=10000&&l.black>=0&&l.white<=1&&l.white-l.black>=.00001&&
          l.gamma>=.1&&l.gamma<=10&&std::abs(l.exposure)<=10&&l.saturation>=0&&l.saturation<=4&&
          l.red>=0&&l.red<=4&&l.green>=0&&l.green<=4&&l.blue>=0&&l.blue<=4,"Invalid mosaic colour or blend parameter");
    check(l.crop.x>=0&&l.crop.y>=0&&l.crop.width>=0&&l.crop.height>=0&&
          ((l.crop.width>0&&l.crop.height>0)||(l.crop.width==0&&l.crop.height==0&&l.crop.x==0&&l.crop.y==0)),"Invalid mosaic crop");
    check(!l.image.empty()&&l.image.string().size()<=4096,"Invalid mosaic input path");
}
cv::Rect region(const MosaicLayer& l,const cv::Mat& image) {
    if(l.crop.width==0)return {0,0,image.cols,image.rows};
    check(l.crop.x<image.cols&&l.crop.y<image.rows&&l.crop.width<=image.cols-l.crop.x&&l.crop.height<=image.rows-l.crop.y,
          "Mosaic crop outside source image");
    return l.crop;
}
cv::Matx23d transform(const MosaicLayer& l) {
    double a=l.angle*CV_PI/180.,c=std::cos(a)*l.scale,s=std::sin(a)*l.scale;
    if(std::abs(c)<1e-12)c=0;
    if(std::abs(s)<1e-12)s=0;
    return {c,-s,l.x,s,c,l.y};
}
}
MosaicResult renderMosaic(const std::vector<MosaicLayer>& layers,int previewEdge) {
    check(!layers.empty()&&layers.size()<=32,"A mosaic requires 1 to 32 layers");
    check(previewEdge==0||(previewEdge>=32&&previewEdge<=2048),"Invalid mosaic preview size");
    double minX=std::numeric_limits<double>::infinity(),minY=minX,maxX=-minX,maxY=maxX;
    int depth=CV_8U;std::uint64_t largestBytes=0;
    struct Source {cv::Rect crop;cv::Size size;int type=0;};std::vector<Source> sources(layers.size());
    for(size_t i=0;i<layers.size();++i) {
        const auto& l=layers[i];validate(l);if(!l.visible)continue;
        auto image=readCompositePng(l.image);auto r=region(l,image);
        sources[i]={r,image.size(),image.type()};if(image.depth()==CV_16U)depth=CV_16U;
        largestBytes=std::max(largestBytes,std::uint64_t(image.total()*image.elemSize()));
        const auto m=transform(l);
        for(double x:{0.,double(r.width)})for(double y:{0.,double(r.height)}) {
            double tx=m(0,0)*x+m(0,1)*y+l.x,ty=m(1,0)*x+m(1,1)*y+l.y;
            minX=std::min(minX,tx);minY=std::min(minY,ty);maxX=std::max(maxX,tx);maxY=std::max(maxY,ty);
        }
    }
    check(std::isfinite(minX),"No visible mosaic layers");
    minX=std::floor(minX);minY=std::floor(minY);maxX=std::ceil(maxX);maxY=std::ceil(maxY);
    const double width=maxX-minX,height=maxY-minY;
    check(width>0&&height>0&&width<=100000&&height<=100000,"Mosaic canvas dimensions exceed limits");
    double ratio=previewEdge?std::min(1.,double(previewEdge)/std::max(width,height)):1.;
    const int cols=std::max(1,int(std::ceil(width*ratio))),rows=std::max(1,int(std::ceil(height*ratio)));
    const auto pixels=std::uint64_t(cols)*rows;
    check(pixels<=200000000,"Mosaic canvas exceeds 200 Mpx; crop or reduce layers");
    int outputDepth=previewEdge?CV_8U:depth;
    check(pixels*(outputDepth==CV_16U?6:3)+largestBytes+128ULL*1024*1024<=2ULL*1024*1024*1024,
          "Mosaic memory budget exceeds 2 GiB; crop or reduce layers");
    MosaicResult result;result.originX=minX;result.originY=minY;result.pixelsPerSourcePixel=ratio;
    result.outlines.resize(layers.size());
    for(size_t i=0;i<layers.size();++i){
        if(!layers[i].visible)continue;
        const auto m=transform(layers[i]);const auto r=sources[i].crop;
        const std::array<cv::Point2d,4> corners{{{0,0},{double(r.width),0},{double(r.width),double(r.height)},{0,double(r.height)}}};
        for(int j=0;j<4;++j){const auto p=corners[size_t(j)];result.outlines[i][size_t(j)]={(m(0,0)*p.x+m(0,1)*p.y+m(0,2)-minX)*ratio,(m(1,0)*p.x+m(1,1)*p.y+m(1,2)-minY)*ratio};}
    }
    result.image=cv::Mat::zeros(rows,cols,CV_MAKETYPE(outputDepth,3));
    for(size_t i=0;i<layers.size();++i) {
        const auto& l=layers[i];if(!l.visible||l.opacity==0)continue;
        auto source=readCompositePng(l.image);
        check(source.size()==sources[i].size&&source.type()==sources[i].type,"Mosaic source changed dimensions or depth");
        auto image=source(sources[i].crop);
        auto m=transform(l);for(int r=0;r<2;++r)for(int c=0;c<3;++c)m(r,c)*=ratio;
        m(0,2)-=minX*ratio;m(1,2)-=minY*ratio;
        cv::Matx23d inverse;cv::invertAffineTransform(m,inverse);
        const double nativeMaximum=source.depth()==CV_16U?65535.:255.,outMaximum=outputDepth==CV_16U?65535.:255.;
        const double gammaExponent=1./l.gamma,exposureGain=std::exp2(l.exposure);
        const std::array<double,3> gains{l.blue*exposureGain,l.green*exposureGain,l.red*exposureGain};
        const bool neutral=l.black==0&&l.white==1&&l.gamma==1&&l.exposure==0&&l.saturation==1&&l.red==1&&l.green==1&&l.blue==1;
        double left=cols,right=0,top=rows,bottom=0;
        for(const auto& point:result.outlines[i]){left=std::min(left,point.x);right=std::max(right,point.x);top=std::min(top,point.y);bottom=std::max(bottom,point.y);}
        const int firstX=std::clamp(int(std::floor(left)),0,cols),lastX=std::clamp(int(std::ceil(right)),0,cols);
        const int firstY=std::clamp(int(std::floor(top)),0,rows),lastY=std::clamp(int(std::ceil(bottom)),0,rows);
        // Bound scratch allocations independently of full scene size. A layer is
        // decoded once per pass, colour conversion and blending use small tiles.
        for(int y=firstY;y<lastY;y+=256)for(int x=firstX;x<lastX;x+=256) {
            cv::Rect tile(x,y,std::min(256,lastX-x),std::min(256,lastY-y));
            auto local=m;local(0,2)-=x;local(1,2)-=y;
            cv::Mat warped,colour;cv::warpAffine(image,warped,local,tile.size(),cv::INTER_LINEAR,cv::BORDER_REPLICATE);
            if(warped.channels()==1)cv::cvtColor(warped,colour,cv::COLOR_GRAY2BGR);else colour=warped;
            colour.convertTo(colour,CV_32F,1./nativeMaximum);
            cv::Mat target;result.image(tile).convertTo(target,CV_32F,1./outMaximum);
            for(int yy=0;yy<tile.height;++yy) {
                auto* values=colour.ptr<cv::Vec3f>(yy);auto* dest=target.ptr<cv::Vec3f>(yy);
                for(int xx=0;xx<tile.width;++xx) {
                    double sx=inverse(0,0)*(x+xx)+inverse(0,1)*(y+yy)+inverse(0,2);
                    double sy=inverse(1,0)*(x+xx)+inverse(1,1)*(y+yy)+inverse(1,2);
                    if(sx<0||sy<0||sx>=image.cols||sy>=image.rows)continue;
                    double alpha=l.opacity;
                    if(l.feather>0)alpha*=std::clamp(std::min({sx+.5,sy+.5,image.cols-.5-sx,image.rows-.5-sy})*l.scale/l.feather,0.,1.);
                    auto v=values[xx];
                    if(neutral){for(int c=0;c<3;++c)dest[xx][c]=float(v[c]*alpha+dest[xx][c]*(1-alpha));continue;}
                    for(int c=0;c<3;++c) {
                        double n=std::clamp((v[c]-l.black)/(l.white-l.black),0.,1.);
                        v[c]=float((l.gamma==1?n:std::pow(n,gammaExponent))*gains[size_t(c)]);
                    }
                    float luminance=.0722f*v[0]+.7152f*v[1]+.2126f*v[2];
                    for(int c=0;c<3;++c) {
                        double adjusted=std::clamp(luminance+l.saturation*(v[c]-luminance),0.,1.);
                        dest[xx][c]=float(adjusted*alpha+dest[xx][c]*(1-alpha));
                    }
                }
            }
            target.convertTo(result.image(tile),result.image.type(),outMaximum);
        }
    }
    return result;
}
std::string mosaicProject(const std::vector<MosaicLayer>& layers) {
    check(!layers.empty()&&layers.size()<=32,"A mosaic requires 1 to 32 layers");
    Json::Value root;root["schema"]="orbyx-mosaic/1";root["geometry"]="manual scene pixels; no geographic certification";
    for(const auto& l:layers) {
        validate(l);Json::Value v;v["image"]=std::filesystem::absolute(l.image).lexically_normal().string();
        v["visible"]=l.visible;
        const double values[]={l.x,l.y,l.scale,l.angle,l.opacity,l.feather,l.black,l.white,l.gamma,l.exposure,l.saturation,l.red,l.green,l.blue};
        const char* keys[]={"x","y","scale","angle","opacity","feather","black","white","gamma","exposure","saturation","red","green","blue"};
        for(int i=0;i<14;++i)v[keys[i]]=values[i];
        for(int n:{l.crop.x,l.crop.y,l.crop.width,l.crop.height})v["crop"].append(n);
        root["layers"].append(v);
    }
    Json::StreamWriterBuilder writer;return Json::writeString(writer,root);
}
std::vector<MosaicLayer> parseMosaicProject(const std::string& text) {
    check(text.size()<=1024*1024,"Mosaic project exceeds 1 MiB");
    Json::CharReaderBuilder reader;reader["rejectDupKeys"]=true;reader["failIfExtra"]=true;
    Json::Value root;std::string error;std::istringstream stream(text);
    check(Json::parseFromStream(reader,stream,&root,&error)&&root.isObject()&&root["schema"]=="orbyx-mosaic/1"&&
          root["layers"].isArray()&&!root["layers"].empty()&&root["layers"].size()<=32,"Invalid mosaic project");
    std::vector<MosaicLayer> layers;
    for(const auto& v:root["layers"]) {
        check(v.isObject()&&v["image"].isString()&&v["visible"].isBool()&&v["crop"].isArray()&&v["crop"].size()==4,"Invalid mosaic layer");
        const auto path=v["image"].asString();check(path.find('\0')==std::string::npos&&std::filesystem::path(path).is_absolute(),"Mosaic paths must be absolute");
        MosaicLayer l;l.image=path;l.visible=v["visible"].asBool();
        double* values[]={&l.x,&l.y,&l.scale,&l.angle,&l.opacity,&l.feather,&l.black,&l.white,&l.gamma,&l.exposure,&l.saturation,&l.red,&l.green,&l.blue};
        const char* keys[]={"x","y","scale","angle","opacity","feather","black","white","gamma","exposure","saturation","red","green","blue"};
        for(int i=0;i<14;++i){check(v[keys[i]].isNumeric(),"Missing mosaic numeric parameter");*values[i]=v[keys[i]].asDouble();}
        for(const auto& n:v["crop"])check(n.isInt(),"Invalid mosaic crop integer");
        l.crop={v["crop"][0].asInt(),v["crop"][1].asInt(),v["crop"][2].asInt(),v["crop"][3].asInt()};
        validate(l);layers.push_back(l);
    }
    return layers;
}
}
