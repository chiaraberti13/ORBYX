// SPDX-License-Identifier: GPL-3.0-only
#include "orbyx/compositor.hpp"
#include <opencv2/calib3d.hpp>
#include <opencv2/features2d.hpp>
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
cv::Mat registrationImage(const cv::Mat& source,const cv::Rect& crop,double& ratio) {
    cv::Mat selected=source(crop),gray;
    if(selected.channels()==3)cv::cvtColor(selected,gray,cv::COLOR_BGR2GRAY);else gray=selected;
    ratio=std::min(1.,1600./std::max(gray.cols,gray.rows));
    cv::Mat small;if(ratio<1)cv::resize(gray,small,{},ratio,ratio,cv::INTER_AREA);else small=gray;
    double low=0,high=0;cv::minMaxLoc(small,&low,&high);
    check(high-low>=8,"Portion has insufficient contrast for automatic overlap");
    cv::Mat normalized;small.convertTo(normalized,CV_8U,255./(high-low),-low*255./(high-low));
    auto clahe=cv::createCLAHE(2.0,{8,8});clahe->apply(normalized,normalized);
    return normalized;
}
struct Features {cv::Size original;double ratio=1;std::vector<cv::KeyPoint> points;cv::Mat descriptors;};
Features features(const MosaicLayer& layer) {
    validate(layer);auto source=readCompositePng(layer.image);auto crop=region(layer,source);
    Features value;value.original=crop.size();auto image=registrationImage(source,crop,value.ratio);
    auto detector=cv::ORB::create(6000,1.2f,8,25,0,2,cv::ORB::HARRIS_SCORE,25,12);
    detector->detectAndCompute(image,cv::noArray(),value.points,value.descriptors);
    check(value.points.size()>=40&&!value.descriptors.empty(),"Too few visual features for automatic overlap");
    return value;
}
double median(std::vector<double> values) {
    check(!values.empty(),"No registration residuals");const auto middle=values.begin()+values.size()/2;
    std::nth_element(values.begin(),middle,values.end());return *middle;
}
struct Candidate {bool valid=false;cv::Matx23d movingToReference;OverlapMatch report;};
Candidate registerPair(const Features& moving,const Features& reference,std::size_t movingIndex,std::size_t referenceIndex) {
    std::vector<std::vector<cv::DMatch>> forward,backward;
    cv::BFMatcher matcher(cv::NORM_HAMMING);matcher.knnMatch(moving.descriptors,reference.descriptors,forward,2);
    matcher.knnMatch(reference.descriptors,moving.descriptors,backward,2);
    std::vector<int> reverse(reference.points.size(),-1);
    for(const auto& pair:backward)if(pair.size()==2&&pair[0].distance<.75f*pair[1].distance)reverse[size_t(pair[0].queryIdx)]=pair[0].trainIdx;
    std::vector<cv::Point2f> from,to;
    for(const auto& pair:forward)if(pair.size()==2&&pair[0].distance<.75f*pair[1].distance&&
       reverse[size_t(pair[0].trainIdx)]==pair[0].queryIdx) {
        from.push_back(moving.points[size_t(pair[0].queryIdx)].pt);to.push_back(reference.points[size_t(pair[0].trainIdx)].pt);
    }
    Candidate result;result.report.moving=movingIndex;result.report.reference=referenceIndex;
    result.report.candidateMatches=int(from.size());if(from.size()<14)return result;
    cv::Mat mask;auto affine=cv::estimateAffinePartial2D(from,to,mask,cv::RANSAC,3.0,3000,.995,20);
    if(affine.empty())return result;
    affine.convertTo(affine,CV_64F);
    std::vector<double> errors;int inliers=0;
    for(int i=0;i<mask.rows;++i)if(mask.at<unsigned char>(i)) {
        ++inliers;const auto& p=from[size_t(i)];const auto& q=to[size_t(i)];
        const double x=affine.at<double>(0,0)*p.x+affine.at<double>(0,1)*p.y+affine.at<double>(0,2);
        const double y=affine.at<double>(1,0)*p.x+affine.at<double>(1,1)*p.y+affine.at<double>(1,2);
        errors.push_back(std::hypot(x-q.x,y-q.y));
    }
    const double inlierFraction=double(inliers)/from.size();if(inliers<12||inlierFraction<.38)return result;
    const double a=affine.at<double>(0,0),b=affine.at<double>(1,0);
    const double smallScale=std::hypot(a,b),scale=smallScale*moving.ratio/reference.ratio;
    if(!std::isfinite(scale)||scale<.5||scale>2)return result;
    cv::Matx23d original{
        affine.at<double>(0,0)*moving.ratio/reference.ratio,affine.at<double>(0,1)*moving.ratio/reference.ratio,affine.at<double>(0,2)/reference.ratio,
        affine.at<double>(1,0)*moving.ratio/reference.ratio,affine.at<double>(1,1)*moving.ratio/reference.ratio,affine.at<double>(1,2)/reference.ratio};
    std::vector<cv::Point2f> movingPolygon{{0,0},{float(moving.original.width),0},{float(moving.original.width),float(moving.original.height)},{0,float(moving.original.height)}};
    std::vector<cv::Point2f> projected;cv::transform(movingPolygon,projected,cv::Mat(original));
    std::vector<cv::Point2f> referencePolygon{{0,0},{float(reference.original.width),0},{float(reference.original.width),float(reference.original.height)},{0,float(reference.original.height)}};
    const double intersection=cv::intersectConvexConvex(projected,referencePolygon,cv::noArray(),true);
    const double movingArea=double(moving.original.area())*scale*scale,referenceArea=reference.original.area();
    const double overlap=intersection/std::max(1.,std::min(movingArea,referenceArea));
    const double residual=median(errors);if(overlap<.03||residual>2.5)return result;
    result.valid=true;result.movingToReference=original;result.report.inliers=inliers;
    result.report.medianError=residual/reference.ratio;result.report.overlapFraction=overlap;
    result.report.confidence=std::clamp(inlierFraction*(1-std::min(1.,residual/4.))*std::min(1.,overlap/.25),0.,1.);
    if(result.report.confidence<.08)result.valid=false;
    return result;
}
cv::Matx33d homogeneous(const cv::Matx23d& value) {
    return {value(0,0),value(0,1),value(0,2),value(1,0),value(1,1),value(1,2),0,0,1};
}
MosaicLayer placed(const MosaicLayer& source,const MosaicLayer& reference,const cv::Matx23d& movingToReference) {
    const auto composed=homogeneous(transform(reference))*homogeneous(movingToReference);
    MosaicLayer result=source;const double a=composed(0,0),b=composed(1,0);
    result.scale=std::hypot(a,b);result.angle=std::atan2(b,a)*180./CV_PI;result.x=composed(0,2);result.y=composed(1,2);
    validate(result);return result;
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
ControlPointSolution alignFromControlPoints(const MosaicLayer& moving,const MosaicLayer& reference,
                                            const std::vector<ControlPointPair>& pairs) {
    validate(moving);validate(reference);
    check(pairs.size()>=3&&pairs.size()<=10000,"Control-point alignment requires 3 to 10000 pairs");
    std::vector<cv::Point2d> from,to;from.reserve(pairs.size());to.reserve(pairs.size());
    double minX=std::numeric_limits<double>::infinity(),minY=minX,maxX=-minX,maxY=maxX;
    for(const auto& pair:pairs){
        for(double value:{pair.moving.x,pair.moving.y,pair.reference.x,pair.reference.y})check(std::isfinite(value)&&std::abs(value)<=1000000,"Invalid control-point coordinate");
        from.push_back(pair.moving);to.push_back(pair.reference);minX=std::min(minX,pair.moving.x);minY=std::min(minY,pair.moving.y);maxX=std::max(maxX,pair.moving.x);maxY=std::max(maxY,pair.moving.y);
    }
    check(std::hypot(maxX-minX,maxY-minY)>=1,"Degenerate control-point geometry");
    cv::Mat mask;auto affine=cv::estimateAffinePartial2D(from,to,mask,cv::RANSAC,3.0,5000,.995,20);
    check(!affine.empty()&&mask.total()==pairs.size(),"Control-point transform could not be estimated");affine.convertTo(affine,CV_64F);
    ControlPointSolution result;std::vector<double> accepted;
    for(size_t i=0;i<pairs.size();++i){
        const auto& p=pairs[i].moving;const auto& q=pairs[i].reference;
        const double x=affine.at<double>(0,0)*p.x+affine.at<double>(0,1)*p.y+affine.at<double>(0,2);
        const double y=affine.at<double>(1,0)*p.x+affine.at<double>(1,1)*p.y+affine.at<double>(1,2);
        const double error=std::hypot(x-q.x,y-q.y);result.residuals.push_back(error);
        if(mask.at<unsigned char>(int(i))){result.inliers.push_back(i);accepted.push_back(error);}
    }
    check(result.inliers.size()>=2,"Too few consistent control points");
    const double a=affine.at<double>(0,0),b=affine.at<double>(1,0),scale=std::hypot(a,b);
    check(std::isfinite(scale)&&scale>=.01&&scale<=100,"Control-point scale exceeds limits");
    result.medianError=median(accepted);result.maxError=*std::max_element(accepted.begin(),accepted.end());
    cv::Matx23d local{affine.at<double>(0,0),affine.at<double>(0,1),affine.at<double>(0,2),affine.at<double>(1,0),affine.at<double>(1,1),affine.at<double>(1,2)};
    result.layer=placed(moving,reference,local);return result;
}
CropProposal proposeUniformBorderCrop(const MosaicLayer& layer) {
    validate(layer);auto source=readCompositePng(layer.image);const auto selected=region(layer,source);cv::Mat image=source(selected),gray;
    if(image.channels()==3)cv::cvtColor(image,gray,cv::COLOR_BGR2GRAY);else gray=image;
    const double ratio=std::min(1.,2048./std::max(gray.cols,gray.rows));cv::Mat small;if(ratio<1)cv::resize(gray,small,{},ratio,ratio,cv::INTER_AREA);else small=gray;
    double low=0,high=0;cv::minMaxLoc(small,&low,&high);check(high-low>=8,"Uniform portion has no reliable content crop");
    cv::Mat normalized;small.convertTo(normalized,CV_8U,255./(high-low),-low*255./(high-low));
    auto activeRows=[&](bool rows){std::vector<unsigned char> active(size_t(rows?normalized.rows:normalized.cols));for(int index=0;index<int(active.size());++index){cv::Mat line=rows?normalized.row(index):normalized.col(index);cv::Scalar mean,deviation;cv::meanStdDev(line,mean,deviation);double minimum=0,maximum=0;cv::minMaxLoc(line,&minimum,&maximum);active[size_t(index)]=deviation[0]>=4&&maximum-minimum>=12;}return active;};
    auto bounds=[](const std::vector<unsigned char>& active){auto first=std::find(active.begin(),active.end(),1);auto last=std::find(active.rbegin(),active.rend(),1);check(first!=active.end(),"Uniform portion has no reliable content crop");return std::pair<int,int>{int(first-active.begin()),int(active.size()-(last-active.rbegin()))};};
    const auto vertical=bounds(activeRows(true)),horizontal=bounds(activeRows(false));
    const int marginX=std::max(2,int(normalized.cols*.01)),marginY=std::max(2,int(normalized.rows*.01));
    const int left=std::max(0,horizontal.first-marginX),right=std::min(normalized.cols,horizontal.second+marginX),top=std::max(0,vertical.first-marginY),bottom=std::min(normalized.rows,vertical.second+marginY);
    cv::Rect local{int(std::floor(left/ratio)),int(std::floor(top/ratio)),int(std::ceil((right-left)/ratio)),int(std::ceil((bottom-top)/ratio))};local&=cv::Rect(0,0,image.cols,image.rows);
    check(local.width>=16&&local.height>=16,"Proposed content crop is too small");const double retained=double(local.area())/selected.area();check(retained>=.01,"Proposed content crop is unreliable");
    CropProposal result;result.original=selected;result.proposed={selected.x+local.x,selected.y+local.y,local.width,local.height};result.retainedFraction=retained;return result;
}
AutoOverlapResult autoAlignOverlaps(const std::vector<MosaicLayer>& layers,std::size_t anchor) {
    check(layers.size()>=2&&layers.size()<=32,"Automatic overlap requires 2 to 32 layers");
    check(anchor<layers.size()&&layers[anchor].visible,"Automatic overlap anchor must be visible");
    AutoOverlapResult output;output.layers=layers;
    std::vector<Features> cache(layers.size());std::vector<bool> available(layers.size()),placedFlags(layers.size());
    for(size_t i=0;i<layers.size();++i)if(layers[i].visible){try{cache[i]=features(layers[i]);available[i]=true;}catch(const std::exception&) {}}
    check(available[anchor],"Anchor has insufficient visual features for automatic overlap");placedFlags[anchor]=true;
    while(true) {
        Candidate best;
        for(size_t moving=0;moving<layers.size();++moving)if(available[moving]&&!placedFlags[moving])
            for(size_t reference=0;reference<layers.size();++reference)if(available[reference]&&placedFlags[reference]) {
                auto candidate=registerPair(cache[moving],cache[reference],moving,reference);
                if(candidate.valid&&(!best.valid||candidate.report.confidence>best.report.confidence))best=candidate;
            }
        if(!best.valid)break;
        output.layers[best.report.moving]=placed(output.layers[best.report.moving],output.layers[best.report.reference],best.movingToReference);
        placedFlags[best.report.moving]=true;output.accepted.push_back(best.report);
    }
    for(size_t i=0;i<layers.size();++i)if(layers[i].visible&&!placedFlags[i])output.unaligned.push_back(i);
    return output;
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
