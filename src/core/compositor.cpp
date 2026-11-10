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
    // A zero width or height simply means "no mask", so partial spin-box edits stay valid.
    check(l.exclude.x>=0&&l.exclude.y>=0&&l.exclude.width>=0&&l.exclude.height>=0&&l.exclude.x<=100000&&l.exclude.y<=100000&&
          l.exclude.width<=100000&&l.exclude.height<=100000,"Invalid mosaic exclude mask");
    check(l.masks.size()<=64,"Too many mosaic polygon masks");std::size_t maskPoints=0;
    for(const auto& mask:l.masks){check(mask.points.size()>=3&&mask.points.size()<=256,"Invalid mosaic polygon mask");maskPoints+=mask.points.size();for(const auto& point:mask.points)check(std::isfinite(point.x)&&std::isfinite(point.y)&&point.x>=0&&point.y>=0&&point.x<=100000&&point.y<=100000,"Invalid mosaic polygon point");}
    check(maskPoints<=4096,"Too many mosaic polygon points");
    check(l.blend==MosaicBlend::Replace||l.blend==MosaicBlend::Feather||l.blend==MosaicBlend::Seam,"Invalid mosaic blend mode");
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
// Select a deterministic minimum-cost cut through the overlap. The calculation
// runs on a bounded working image (at most 2048 px on the long side); the result
// is sampled by the tiled native-depth renderer. An empty result means that the
// overlap has no continuous path and the caller must use the feather fallback.
cv::Mat minimumCostSeam(const cv::Mat& target,const cv::Mat& incomingImage,
                        const cv::Mat& existing,const cv::Mat& incoming,double transition) {
    check(target.type()==CV_8U&&incomingImage.type()==CV_8U&&existing.type()==CV_8U&&incoming.type()==CV_8U&&
          target.size()==incomingImage.size()&&target.size()==existing.size()&&target.size()==incoming.size(),
          "Invalid optimized seam buffers");
    cv::Mat overlap;cv::bitwise_and(existing,incoming,overlap);
    std::vector<cv::Point> overlapPoints;cv::findNonZero(overlap,overlapPoints);
    cv::Mat base;incoming.convertTo(base,CV_32F,1./255.);
    if(overlapPoints.empty())return base;
    const auto originalBounds=cv::boundingRect(overlapPoints);
    const bool transpose=originalBounds.width>originalBounds.height;
    cv::Mat a=target,b=incomingImage,oldMask=existing,newMask=incoming,common=overlap;
    if(transpose){cv::transpose(a,a);cv::transpose(b,b);cv::transpose(oldMask,oldMask);cv::transpose(newMask,newMask);cv::transpose(common,common);}
    std::vector<cv::Point> points;cv::findNonZero(common,points);const auto bounds=cv::boundingRect(points);
    cv::Mat difference;cv::absdiff(a,b,difference);difference.convertTo(difference,CV_32F);
    cv::Mat ax,ay,bx,by,gradientX,gradientY;
    cv::Sobel(a,ax,CV_32F,1,0,3);cv::Sobel(a,ay,CV_32F,0,1,3);
    cv::Sobel(b,bx,CV_32F,1,0,3);cv::Sobel(b,by,CV_32F,0,1,3);
    cv::absdiff(ax,bx,gradientX);cv::absdiff(ay,by,gradientY);
    difference+=.15f*(gradientX+gradientY);
    constexpr float infinity=1e30f;const int width=bounds.width,height=bounds.height;
    std::vector<float> previous(size_t(width),infinity),current(size_t(width),infinity);
    std::vector<short> parent(size_t(width)*height,short(-1));
    for(int x=0;x<width;++x)if(common.at<unsigned char>(bounds.y,bounds.x+x))previous[size_t(x)]=difference.at<float>(bounds.y,bounds.x+x);
    for(int y=1;y<height;++y){std::fill(current.begin(),current.end(),infinity);for(int x=0;x<width;++x){
        if(!common.at<unsigned char>(bounds.y+y,bounds.x+x))continue;
        float best=infinity;int bestX=-1;
        for(int delta=-2;delta<=2;++delta){const int candidate=x+delta;if(candidate<0||candidate>=width)continue;const float value=previous[size_t(candidate)]+2.f*std::abs(delta);if(value<best){best=value;bestX=candidate;}}
        if(bestX>=0&&best<infinity/2){current[size_t(x)]=best+difference.at<float>(bounds.y+y,bounds.x+x);parent[size_t(y)*width+x]=short(bestX);}
    }previous.swap(current);}
    const auto end=std::min_element(previous.begin(),previous.end());if(end==previous.end()||*end>=infinity/2)return {};
    std::vector<int> path(static_cast<size_t>(height));path.back()=int(end-previous.begin());
    for(int y=height-1;y>0;--y){const short prior=parent[size_t(y)*width+path[size_t(y)]];if(prior<0)return {};path[size_t(y-1)]=prior;}
    cv::Mat onlyNew;cv::bitwise_not(oldMask,onlyNew);cv::bitwise_and(onlyNew,newMask,onlyNew);
    cv::Moments location=cv::moments(onlyNew,true);if(location.m00==0)location=cv::moments(newMask,true);
    double meanPath=0;for(int value:path)meanPath+=bounds.x+value;meanPath/=path.size();
    const bool chooseRight=location.m00==0||location.m10/location.m00>=meanPath;
    cv::Mat alpha;newMask.convertTo(alpha,CV_32F,1./255.);
    for(int y=0;y<height;++y){const int cut=bounds.x+path[size_t(y)];auto* values=alpha.ptr<float>(bounds.y+y);const auto* valid=common.ptr<unsigned char>(bounds.y+y);for(int x=bounds.x;x<bounds.x+width;++x)if(valid[x])values[x]=(chooseRight?x>=cut:x<=cut)?1.f:0.f;}
    if(transition>.5){cv::Mat selected,rejected,inside,outside;cv::compare(alpha,.5,selected,cv::CMP_GT);cv::bitwise_not(selected,rejected);cv::distanceTransform(selected,inside,cv::DIST_L2,cv::DIST_MASK_PRECISE);cv::distanceTransform(rejected,outside,cv::DIST_L2,cv::DIST_MASK_PRECISE);for(int y=0;y<alpha.rows;++y){auto* values=alpha.ptr<float>(y);const auto* overlapValues=common.ptr<unsigned char>(y);const auto* in=inside.ptr<float>(y);const auto* out=outside.ptr<float>(y);for(int x=0;x<alpha.cols;++x)if(overlapValues[x])values[x]=std::clamp(.5f+(in[x]-out[x])/float(2*transition),0.f,1.f);}}
    if(transpose)cv::transpose(alpha,alpha);
    return alpha;
}
bool acceptsVisualSample(const MosaicLayer& layer,const cv::Point2d& point,const cv::Size& size) {
    if(point.x<0||point.y<0||point.x>=size.width||point.y>=size.height)return false;
    if(layer.exclude.width>0&&layer.exclude.height>0&&point.x>=layer.exclude.x&&point.y>=layer.exclude.y&&point.x<layer.exclude.x+layer.exclude.width&&point.y<layer.exclude.y+layer.exclude.height)return false;
    bool hasInclude=false,included=false;
    for(const auto& mask:layer.masks){std::vector<cv::Point2f> polygon;polygon.reserve(mask.points.size());for(const auto& value:mask.points)polygon.emplace_back(float(value.x),float(value.y));const bool inside=cv::pointPolygonTest(polygon,point,false)>=0;if(mask.include){hasInclude=true;included=included||inside;}else if(inside)return false;}
    return !hasInclude||included;
}
cv::Vec3d visualPixel(const cv::Mat& image,int x,int y,const MosaicLayer& layer) {
    cv::Vec3d value;if(image.channels()==1){const double v=image.depth()==CV_16U?image.at<unsigned short>(y,x):image.at<unsigned char>(y,x);value={v,v,v};}
    else if(image.depth()==CV_16U){const auto v=image.at<cv::Vec3w>(y,x);value={double(v[0]),double(v[1]),double(v[2])};}
    else {const auto v=image.at<cv::Vec3b>(y,x);value={double(v[0]),double(v[1]),double(v[2])};}
    const double maximum=image.depth()==CV_16U?65535.:255.,exposure=std::exp2(layer.exposure),gamma=1./layer.gamma;const std::array<double,3> gains{layer.blue*exposure,layer.green*exposure,layer.red*exposure};
    for(int channel=0;channel<3;++channel){const double normalized=std::clamp((value[channel]/maximum-layer.black)/(layer.white-layer.black),0.,1.);value[channel]=(layer.gamma==1?normalized:std::pow(normalized,gamma))*gains[size_t(channel)];}
    const double luminance=.0722*value[0]+.7152*value[1]+.2126*value[2];for(int channel=0;channel<3;++channel)value[channel]=std::clamp(luminance+layer.saturation*(value[channel]-luminance),0.,1.);return value;
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
    const double seamScale=std::min(1.,2048./std::max(cols,rows));
    const cv::Size seamSize(std::max(1,int(std::ceil(cols*seamScale))),std::max(1,int(std::ceil(rows*seamScale))));
    cv::Mat seamCoverage=cv::Mat::zeros(seamSize,CV_8U);
    for(size_t i=0;i<layers.size();++i) {
        const auto& l=layers[i];if(!l.visible||(l.blend==MosaicBlend::Feather&&l.opacity==0))continue;
        auto source=readCompositePng(l.image);
        check(source.size()==sources[i].size&&source.type()==sources[i].type,"Mosaic source changed dimensions or depth");
        auto image=source(sources[i].crop);
        const cv::Rect exclude=l.exclude;
        const bool masked=exclude.width>0&&exclude.height>0;
        if(masked)check(exclude.x+exclude.width<=image.cols&&exclude.y+exclude.height<=image.rows,"Mosaic exclude mask outside layer region");
        auto m=transform(l);for(int r=0;r<2;++r)for(int c=0;c<3;++c)m(r,c)*=ratio;
        m(0,2)-=minX*ratio;m(1,2)-=minY*ratio;
        bool hasIncludeMasks=false;std::vector<std::vector<cv::Point>> includeMasks,excludeMasks;
        for(const auto& mask:l.masks){std::vector<cv::Point> polygon;polygon.reserve(mask.points.size());for(const auto& point:mask.points){check(point.x<=image.cols&&point.y<=image.rows,"Mosaic polygon mask outside layer region");polygon.emplace_back(cvRound(m(0,0)*point.x+m(0,1)*point.y+m(0,2)),cvRound(m(1,0)*point.x+m(1,1)*point.y+m(1,2)));}if(mask.include){hasIncludeMasks=true;includeMasks.push_back(std::move(polygon));}else excludeMasks.push_back(std::move(polygon));}
        cv::Mat layerCoverage=cv::Mat::zeros(seamSize,CV_8U);std::vector<cv::Point> boundary;
        for(const auto& point:result.outlines[i])boundary.emplace_back(cvRound(point.x*seamScale),cvRound(point.y*seamScale));
        cv::fillConvexPoly(layerCoverage,boundary,cv::Scalar(255));
        auto scaledPolygons=[&](const std::vector<std::vector<cv::Point>>& polygons){std::vector<std::vector<cv::Point>> scaled;for(const auto& polygon:polygons){std::vector<cv::Point> value;for(const auto& point:polygon)value.emplace_back(cvRound(point.x*seamScale),cvRound(point.y*seamScale));scaled.push_back(std::move(value));}return scaled;};
        const auto scaledIncludes=scaledPolygons(includeMasks),scaledExcludes=scaledPolygons(excludeMasks);
        if(hasIncludeMasks){cv::Mat included=cv::Mat::zeros(seamSize,CV_8U);cv::fillPoly(included,scaledIncludes,cv::Scalar(255));cv::bitwise_and(layerCoverage,included,layerCoverage);}
        if(!scaledExcludes.empty())cv::fillPoly(layerCoverage,scaledExcludes,cv::Scalar(0));
        if(masked){std::vector<cv::Point> polygon;for(const cv::Point2d point:std::vector<cv::Point2d>{{double(exclude.x),double(exclude.y)},{double(exclude.x+exclude.width),double(exclude.y)},{double(exclude.x+exclude.width),double(exclude.y+exclude.height)},{double(exclude.x),double(exclude.y+exclude.height)}})polygon.emplace_back(cvRound((m(0,0)*point.x+m(0,1)*point.y+m(0,2))*seamScale),cvRound((m(1,0)*point.x+m(1,1)*point.y+m(1,2))*seamScale));cv::fillConvexPoly(layerCoverage,polygon,cv::Scalar(0));}
        cv::Mat seamAlpha;bool seamReady=false;
        if(l.blend==MosaicBlend::Seam){
            cv::Mat sourceGray;if(image.channels()==3)cv::cvtColor(image,sourceGray,cv::COLOR_BGR2GRAY);else sourceGray=image;
            cv::Matx23d seamTransform=m;for(int r=0;r<2;++r)for(int c=0;c<3;++c)seamTransform(r,c)*=seamScale;
            cv::Mat sourceSmall;cv::warpAffine(sourceGray,sourceSmall,seamTransform,seamSize,cv::INTER_LINEAR,cv::BORDER_REPLICATE);if(sourceSmall.depth()==CV_16U)sourceSmall.convertTo(sourceSmall,CV_8U,1./257.);
            cv::Mat targetSmall;cv::resize(result.image,targetSmall,seamSize,0,0,cv::INTER_AREA);if(targetSmall.channels()==3)cv::cvtColor(targetSmall,targetSmall,cv::COLOR_BGR2GRAY);if(targetSmall.depth()==CV_16U)targetSmall.convertTo(targetSmall,CV_8U,1./257.);
            seamAlpha=minimumCostSeam(targetSmall,sourceSmall,seamCoverage,layerCoverage,l.feather*l.scale*ratio*seamScale);seamReady=!seamAlpha.empty();if(!seamReady)++result.seamFallbacks;
        }
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
            cv::Mat polygonMask;if(hasIncludeMasks||!excludeMasks.empty()){polygonMask=cv::Mat(tile.size(),CV_8U,cv::Scalar(hasIncludeMasks?0:255));if(hasIncludeMasks)cv::fillPoly(polygonMask,includeMasks,cv::Scalar(255),cv::LINE_8,0,{-x,-y});if(!excludeMasks.empty())cv::fillPoly(polygonMask,excludeMasks,cv::Scalar(0),cv::LINE_8,0,{-x,-y});}
            for(int yy=0;yy<tile.height;++yy) {
                auto* values=colour.ptr<cv::Vec3f>(yy);auto* dest=target.ptr<cv::Vec3f>(yy);
                for(int xx=0;xx<tile.width;++xx) {
                    double sx=inverse(0,0)*(x+xx)+inverse(0,1)*(y+yy)+inverse(0,2);
                    double sy=inverse(1,0)*(x+xx)+inverse(1,1)*(y+yy)+inverse(1,2);
                    if(sx<0||sy<0||sx>=image.cols||sy>=image.rows)continue;
                    // Non-destructive exclude mask: this region does not contribute, revealing layers below.
                    if(masked&&sx>=exclude.x&&sx<exclude.x+exclude.width&&sy>=exclude.y&&sy<exclude.y+exclude.height)continue;
                    if(!polygonMask.empty()&&polygonMask.at<unsigned char>(yy,xx)==0)continue;
                    double alpha=l.blend==MosaicBlend::Replace?1.:l.opacity;
                    if(l.blend==MosaicBlend::Seam&&seamReady){const int seamX=std::clamp(int((x+xx+.5)*seamScale),0,seamAlpha.cols-1),seamY=std::clamp(int((y+yy+.5)*seamScale),0,seamAlpha.rows-1);alpha=seamAlpha.at<float>(seamY,seamX);}
                    if((l.blend==MosaicBlend::Feather||(l.blend==MosaicBlend::Seam&&!seamReady))&&l.feather>0)alpha*=std::clamp(std::min({sx+.5,sy+.5,image.cols-.5-sx,image.rows-.5-sy})*l.scale/l.feather,0.,1.);
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
        cv::bitwise_or(seamCoverage,layerCoverage,seamCoverage);
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
    const auto movingRegion=region(moving,readCompositePng(moving.image)),referenceRegion=region(reference,readCompositePng(reference.image));
    auto coverage=[&](bool useMoving,const cv::Rect& bounds){double x0=std::numeric_limits<double>::infinity(),y0=x0,x1=-x0,y1=-x0;for(const auto index:result.inliers){const auto& point=useMoving?pairs[index].moving:pairs[index].reference;x0=std::min(x0,point.x);y0=std::min(y0,point.y);x1=std::max(x1,point.x);y1=std::max(y1,point.y);}const double spanX=std::clamp((x1-x0)/bounds.width,0.,1.),spanY=std::clamp((y1-y0)/bounds.height,0.,1.);return std::array<double,3>{spanX*spanY,spanX,spanY};};
    const auto movingSpread=coverage(true,movingRegion),referenceSpread=coverage(false,referenceRegion);
    result.movingCoverage=movingSpread[0];result.referenceCoverage=referenceSpread[0];
    result.spatiallyConcentrated=movingSpread[0]<.05||referenceSpread[0]<.05||movingSpread[1]<.15||movingSpread[2]<.15||referenceSpread[1]<.15||referenceSpread[2]<.15;
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
VisualCompensation proposeVisualCompensation(const std::vector<MosaicLayer>& layers,std::size_t movingIndex,std::size_t referenceIndex) {
    check(layers.size()>=2&&layers.size()<=32&&movingIndex<layers.size()&&referenceIndex<layers.size()&&movingIndex!=referenceIndex,"Invalid visual compensation selection");
    const auto& moving=layers[movingIndex];const auto& reference=layers[referenceIndex];validate(moving);validate(reference);
    check(moving.visible&&reference.visible,"Visual compensation requires visible layers");
    check(moving.red>.01&&moving.green>.01&&moving.blue>.01,"Reset zero colour gains before visual compensation");
    auto movingSource=readCompositePng(moving.image),referenceSource=readCompositePng(reference.image);const auto movingRegion=region(moving,movingSource),referenceRegion=region(reference,referenceSource);
    cv::Mat movingImage=movingSource(movingRegion),referenceImage=referenceSource(referenceRegion);const auto movingTransform=transform(moving),referenceTransform=transform(reference);
    auto sceneBounds=[](const cv::Matx23d& matrix,const cv::Size& size){double left=std::numeric_limits<double>::infinity(),top=left,right=-left,bottom=-left;for(double x:{0.,double(size.width)})for(double y:{0.,double(size.height)}){const double px=matrix(0,0)*x+matrix(0,1)*y+matrix(0,2),py=matrix(1,0)*x+matrix(1,1)*y+matrix(1,2);left=std::min(left,px);right=std::max(right,px);top=std::min(top,py);bottom=std::max(bottom,py);}return cv::Rect2d(left,top,right-left,bottom-top);};
    const auto overlap=sceneBounds(movingTransform,movingImage.size())&sceneBounds(referenceTransform,referenceImage.size());check(overlap.width>=2&&overlap.height>=2,"Selected layers have no visual overlap");
    cv::Matx23d movingInverse,referenceInverse;cv::invertAffineTransform(movingTransform,movingInverse);cv::invertAffineTransform(referenceTransform,referenceInverse);
    const double area=overlap.width*overlap.height;const int step=std::max(1,int(std::ceil(std::sqrt(area/150000.))));std::array<std::vector<double>,3> logRatios;
    for(int y=int(std::floor(overlap.y));y<int(std::ceil(overlap.y+overlap.height));y+=step)for(int x=int(std::floor(overlap.x));x<int(std::ceil(overlap.x+overlap.width));x+=step){const double sceneX=x+.5,sceneY=y+.5;const cv::Point2d mp{movingInverse(0,0)*sceneX+movingInverse(0,1)*sceneY+movingInverse(0,2),movingInverse(1,0)*sceneX+movingInverse(1,1)*sceneY+movingInverse(1,2)},rp{referenceInverse(0,0)*sceneX+referenceInverse(0,1)*sceneY+referenceInverse(0,2),referenceInverse(1,0)*sceneX+referenceInverse(1,1)*sceneY+referenceInverse(1,2)};if(!acceptsVisualSample(moving,mp,movingImage.size())||!acceptsVisualSample(reference,rp,referenceImage.size()))continue;const auto a=visualPixel(movingImage,std::clamp(int(mp.x),0,movingImage.cols-1),std::clamp(int(mp.y),0,movingImage.rows-1),moving),b=visualPixel(referenceImage,std::clamp(int(rp.x),0,referenceImage.cols-1),std::clamp(int(rp.y),0,referenceImage.rows-1),reference);const double lumA=.0722*a[0]+.7152*a[1]+.2126*a[2],lumB=.0722*b[0]+.7152*b[1]+.2126*b[2];if(lumA<.025||lumB<.025||lumA>.975||lumB>.975)continue;for(int channel=0;channel<3;++channel)if(a[channel]>=.015&&b[channel]>=.015&&a[channel]<=.985&&b[channel]<=.985)logRatios[size_t(channel)].push_back(std::log(std::clamp(b[channel]/a[channel],.25,4.)));}
    const std::size_t samples=std::min({logRatios[0].size(),logRatios[1].size(),logRatios[2].size()});check(samples>=64,"Too few reliable overlap pixels for visual compensation");
    const std::array<double,3> logs{median(logRatios[0]),median(logRatios[1]),median(logRatios[2])};const double common=.0722*logs[0]+.7152*logs[1]+.2126*logs[2];
    VisualCompensation result;result.layer=moving;result.samples=samples;result.layer.exposure=std::clamp(moving.exposure+common/std::log(2.),-10.,10.);result.layer.blue=std::clamp(moving.blue*std::exp(logs[0]-common),.01,4.);result.layer.green=std::clamp(moving.green*std::exp(logs[1]-common),.01,4.);result.layer.red=std::clamp(moving.red*std::exp(logs[2]-common),.01,4.);result.exposureDelta=result.layer.exposure-moving.exposure;result.blueMultiplier=result.layer.blue/moving.blue;result.greenMultiplier=result.layer.green/moving.green;result.redMultiplier=result.layer.red/moving.red;validate(result.layer);return result;
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
        v["visible"]=l.visible;v["blend"]=l.blend==MosaicBlend::Replace?"replace":l.blend==MosaicBlend::Seam?"seam":"feather";
        const double values[]={l.x,l.y,l.scale,l.angle,l.opacity,l.feather,l.black,l.white,l.gamma,l.exposure,l.saturation,l.red,l.green,l.blue};
        const char* keys[]={"x","y","scale","angle","opacity","feather","black","white","gamma","exposure","saturation","red","green","blue"};
        for(int i=0;i<14;++i)v[keys[i]]=values[i];
        for(int n:{l.crop.x,l.crop.y,l.crop.width,l.crop.height})v["crop"].append(n);
        if(l.exclude.width>0&&l.exclude.height>0)for(int n:{l.exclude.x,l.exclude.y,l.exclude.width,l.exclude.height})v["exclude"].append(n);
        for(const auto& mask:l.masks){Json::Value item;item["mode"]=mask.include?"include":"exclude";for(const auto& point:mask.points){Json::Value coordinates;coordinates.append(point.x);coordinates.append(point.y);item["points"].append(coordinates);}v["masks"].append(item);}
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
        if(v.isMember("blend")){check(v["blend"].isString(),"Invalid mosaic blend mode");const auto blend=v["blend"].asString();check(blend=="replace"||blend=="feather"||blend=="seam","Invalid mosaic blend mode");l.blend=blend=="replace"?MosaicBlend::Replace:blend=="seam"?MosaicBlend::Seam:MosaicBlend::Feather;}
        double* values[]={&l.x,&l.y,&l.scale,&l.angle,&l.opacity,&l.feather,&l.black,&l.white,&l.gamma,&l.exposure,&l.saturation,&l.red,&l.green,&l.blue};
        const char* keys[]={"x","y","scale","angle","opacity","feather","black","white","gamma","exposure","saturation","red","green","blue"};
        for(int i=0;i<14;++i){check(v[keys[i]].isNumeric(),"Missing mosaic numeric parameter");*values[i]=v[keys[i]].asDouble();}
        for(const auto& n:v["crop"])check(n.isInt(),"Invalid mosaic crop integer");
        l.crop={v["crop"][0].asInt(),v["crop"][1].asInt(),v["crop"][2].asInt(),v["crop"][3].asInt()};
        if(v.isMember("exclude")){ // Optional: older projects omit it and keep no mask.
            check(v["exclude"].isArray()&&v["exclude"].size()==4,"Invalid mosaic exclude mask");
            for(const auto& n:v["exclude"])check(n.isInt(),"Invalid mosaic exclude integer");
            l.exclude={v["exclude"][0].asInt(),v["exclude"][1].asInt(),v["exclude"][2].asInt(),v["exclude"][3].asInt()};
        }
        if(v.isMember("masks")){check(v["masks"].isArray()&&v["masks"].size()<=64,"Invalid mosaic polygon masks");for(const auto& item:v["masks"]){check(item.isObject()&&item["mode"].isString()&&item["points"].isArray(),"Invalid mosaic polygon mask");const auto mode=item["mode"].asString();check(mode=="include"||mode=="exclude","Invalid mosaic polygon mode");MosaicMask mask;mask.include=mode=="include";for(const auto& coordinates:item["points"]){check(coordinates.isArray()&&coordinates.size()==2&&coordinates[0].isNumeric()&&coordinates[1].isNumeric(),"Invalid mosaic polygon point");mask.points.emplace_back(coordinates[0].asDouble(),coordinates[1].asDouble());}l.masks.push_back(std::move(mask));}}
        validate(l);layers.push_back(l);
    }
    return layers;
}
}
