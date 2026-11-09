// SPDX-License-Identifier: GPL-3.0-only
#include "orbyx/compositor.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <unistd.h>
namespace fs=std::filesystem;
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F> void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}check(threw,"Invalid input accepted");}
int main(){
    auto root=fs::temp_directory_path()/("orbyx-mosaic-"+std::to_string(getpid()));
    check(fs::create_directory(root),"Temporary fixture exists");
    try {
        cv::Mat a(7,9,CV_16UC3,cv::Scalar(1025,2051,4007));
        cv::Mat b(7,9,CV_16UC3,cv::Scalar(10003,20009,30011));
        cv::Mat c(7,9,CV_16UC3,cv::Scalar(43001,52003,63007));
        for(auto pair:{std::make_pair("a.png",a),std::make_pair("b.png",b),std::make_pair("c.png",c)})check(cv::imwrite((root/pair.first).string(),pair.second),"Write fixture");
        orbyx::MosaicLayer l;l.image=root/"a.png";auto m=l;m.image=root/"b.png";m.x=9;auto n=l;n.image=root/"c.png";n.y=7;
        auto result=orbyx::renderMosaic({l,m,n});
        check(result.image.size()==cv::Size(18,14)&&result.image.type()==CV_16UC3,"Three-portion canvas/depth incorrect");
        check(cv::norm(result.image(cv::Rect(0,0,9,7)),a,cv::NORM_INF)==0,"16-bit low bits lost");
        check(cv::norm(result.image(cv::Rect(9,0,9,7)),b,cv::NORM_INF)==0,"Horizontal portion misplaced");
        check(cv::norm(result.image(cv::Rect(0,7,9,7)),c,cv::NORM_INF)==0,"Vertical portion misplaced");
        check(cv::countNonZero(result.image(cv::Rect(9,7,9,7)).reshape(1))==0,"Empty canvas invented");
        m.x=0;m.opacity=.5;auto blend=orbyx::renderMosaic({l,m}).image.at<cv::Vec3w>(3,3);
        for(int k=0;k<3;++k)check(std::abs(int(blend[k])-int(std::nearbyint((a.at<cv::Vec3w>(3,3)[k]+b.at<cv::Vec3w>(3,3)[k])*.5)))<=1,"Opacity oracle failed");
        m.opacity=1;m.feather=4;
        auto feather=orbyx::renderMosaic({l,m}).image;
        check(feather.at<cv::Vec3w>(0,0)[0]<feather.at<cv::Vec3w>(3,3)[0],"Feather not applied at boundary");
        m=l;m.x=-5;m.y=-3;auto negative=orbyx::renderMosaic({m});
        check(negative.originX==-5&&negative.originY==-3&&cv::norm(negative.image,a,cv::NORM_INF)==0,"Negative-origin placement incorrect");
        m=l;m.crop={2,1,4,3};check(orbyx::renderMosaic({m}).image.size()==cv::Size(4,3),"Crop failed");
        m.angle=90;auto rotate=orbyx::renderMosaic({m});check(rotate.image.cols<=4&&rotate.image.rows<=5,"Rotation canvas incorrect");
        m=l;m.scale=2;check(orbyx::renderMosaic({m}).image.size()==cv::Size(18,14),"Scale failed");
        m=l;m.saturation=0;auto grey=orbyx::renderMosaic({m}).image.at<cv::Vec3w>(3,3);check(grey[0]==grey[1]&&grey[1]==grey[2],"Saturation failed");
        m=l;m.exposure=1;auto exposure=orbyx::renderMosaic({m}).image.at<cv::Vec3w>(3,3);check(exposure[0]==2050,"Exposure oracle failed");
        m=l;m.visible=false;check(cv::norm(orbyx::renderMosaic({l,m}).image,a,cv::NORM_INF)==0,"Hidden layer rendered");
        auto project=orbyx::mosaicProject({l,n});auto loaded=orbyx::parseMosaicProject(project);
        check(cv::norm(orbyx::renderMosaic(loaded).image,orbyx::renderMosaic({l,n}).image,cv::NORM_INF)==0,"Project round trip changed pixels");
        rejects([&]{orbyx::parseMosaicProject(project+"{}");});
        rejects([&]{orbyx::parseMosaicProject("{\"schema\":\"orbyx-mosaic/1\",\"schema\":\"bad\",\"layers\":[]}");});
        m=l;m.x=std::numeric_limits<double>::infinity();rejects([&]{orbyx::renderMosaic({m});});
        m=l;m.crop={99,0,1,1};rejects([&]{orbyx::renderMosaic({m});});
        m=l;m.white=m.black;rejects([&]{orbyx::renderMosaic({m});});
        m=l;m.x=900000;rejects([&]{orbyx::renderMosaic({l,m});});
        rejects([&]{orbyx::renderMosaic(std::vector<orbyx::MosaicLayer>(33,l));});
        fs::create_symlink(root/"a.png",root/"link.png");m=l;m.image=root/"link.png";rejects([&]{orbyx::renderMosaic({m});});
        auto file=root/"result.png";orbyx::writeCompositePng(result.image,file);rejects([&]{orbyx::writeCompositePng(result.image,file);});
        check(cv::norm(cv::imread(file.string(),cv::IMREAD_UNCHANGED),result.image,cv::NORM_INF)==0,"PNG depth/pixels changed");
        check(cv::norm(cv::imread((root/"a.png").string(),cv::IMREAD_UNCHANGED),a,cv::NORM_INF)==0,"Source overwritten");
        auto preview=orbyx::renderMosaic({l,n},32);check(preview.image.depth()==CV_8U,"Preview is not explicitly 8-bit");
        // Automatic overlap works on arbitrary portions, including a chain in
        // two directions; a featureless part is retained for manual editing.
        cv::Mat scene(520,760,CV_8UC3,cv::Scalar(8,8,8));cv::RNG random(0x4f524259);
        for(int i=0;i<450;++i){cv::Point point(random.uniform(8,752),random.uniform(8,512));cv::Scalar colour(random.uniform(35,255),random.uniform(35,255),random.uniform(35,255));cv::circle(scene,point,random.uniform(2,9),colour,-1,cv::LINE_AA);}
        for(int i=0;i<80;++i)cv::line(scene,{random.uniform(0,760),random.uniform(0,520)},{random.uniform(0,760),random.uniform(0,520)},cv::Scalar(random.uniform(20,240),random.uniform(20,240),random.uniform(20,240)),1,cv::LINE_AA);
        cv::putText(scene,"ORBYX AUTO OVERLAP",{55,255},cv::FONT_HERSHEY_SIMPLEX,1.2,{255,255,255},3,cv::LINE_AA);
        const std::array<cv::Rect,3> portions{{{0,0,430,360},{240,70,430,360},{130,240,430,280}}};
        std::vector<orbyx::MosaicLayer> automatic;
        for(size_t i=0;i<portions.size();++i){auto file=root/("portion-"+std::to_string(i)+".png");cv::imwrite(file.string(),scene(portions[i]));orbyx::MosaicLayer part;part.image=file;automatic.push_back(part);}
        auto blankFile=root/"blank.png";cv::imwrite(blankFile.string(),cv::Mat(240,320,CV_8UC3,cv::Scalar(30,30,30)));orbyx::MosaicLayer blank;blank.image=blankFile;automatic.push_back(blank);
        auto arranged=orbyx::autoAlignOverlaps(automatic,0);
        check(arranged.accepted.size()==2&&arranged.unaligned==std::vector<std::size_t>{3},"Automatic overlap did not distinguish connected and unmatched portions");
        for(size_t i=1;i<3;++i){check(std::abs(arranged.layers[i].x-portions[i].x)<=2&&std::abs(arranged.layers[i].y-portions[i].y)<=2,"Automatic translation outside tolerance");check(std::abs(arranged.layers[i].scale-1)<.01&&std::abs(arranged.layers[i].angle)<.3,"Automatic geometry invented rotation or scale");}
        for(const auto& match:arranged.accepted)check(match.inliers>=12&&match.confidence>0&&match.overlapFraction>=.03,"Automatic overlap evidence missing");
        check(automatic[1].x==0&&automatic[1].y==0,"Automatic proposal modified caller state");
        // Recover rotation and scale, not only translation.
        auto referenceFile=root/"reference.png";cv::imwrite(referenceFile.string(),scene(cv::Rect(80,60,520,400)));
        cv::Mat rotation=cv::getRotationMatrix2D({260,200},6,.94),rotated;cv::warpAffine(scene(cv::Rect(80,60,520,400)),rotated,rotation,{520,400},cv::INTER_LINEAR,cv::BORDER_REFLECT);
        auto rotatedFile=root/"rotated.png";cv::imwrite(rotatedFile.string(),rotated);
        orbyx::MosaicLayer reference;reference.image=referenceFile;orbyx::MosaicLayer moving;moving.image=rotatedFile;
        auto recovered=orbyx::autoAlignOverlaps({reference,moving});cv::Mat inverse;cv::invertAffineTransform(rotation,inverse);
        check(recovered.accepted.size()==1&&recovered.unaligned.empty(),"Rotation/scale overlap was not accepted");
        check(std::abs(recovered.layers[1].scale-std::hypot(inverse.at<double>(0,0),inverse.at<double>(1,0)))<.025,"Automatic scale outside tolerance");
        check(std::abs(recovered.layers[1].angle-std::atan2(inverse.at<double>(1,0),inverse.at<double>(0,0))*180/CV_PI)<.7,"Automatic rotation outside tolerance");
        check(std::abs(recovered.layers[1].x-inverse.at<double>(0,2))<6&&std::abs(recovered.layers[1].y-inverse.at<double>(1,2))<6,"Automatic rotated translation outside tolerance");
        auto noMatch=orbyx::autoAlignOverlaps({automatic[0],blank});
        check(noMatch.accepted.empty()&&noMatch.unaligned==std::vector<std::size_t>{1},"Featureless portion was not retained for manual editing");
        // User-reviewed control points recover a known similarity transform,
        // reject one gross outlier and expose residuals for every pair.
        reference.x=100;reference.y=50;reference.scale=1.2;reference.angle=10;moving.x=-400;moving.y=700;
        const double relativeScale=.92,relativeAngle=-4*CV_PI/180.,rc=std::cos(relativeAngle)*relativeScale,rs=std::sin(relativeAngle)*relativeScale;
        std::vector<orbyx::ControlPointPair> controlPoints;
        for(const cv::Point2d point:std::vector<cv::Point2d>{{10,20},{160,25},{25,140},{190,155},{90,70},{220,90},{55,190},{175,205}})
            controlPoints.push_back({point,{rc*point.x-rs*point.y+25,rs*point.x+rc*point.y-12}});
        controlPoints.push_back({{120,120},{900,-600}});
        auto controlled=orbyx::alignFromControlPoints(moving,reference,controlPoints);
        const double referenceAngle=10*CV_PI/180.,expectedX=100+1.2*(std::cos(referenceAngle)*25-std::sin(referenceAngle)*-12),expectedY=50+1.2*(std::sin(referenceAngle)*25+std::cos(referenceAngle)*-12);
        check(std::abs(controlled.layer.scale-1.2*relativeScale)<1e-5&&std::abs(controlled.layer.angle-6)<1e-5,"Control points recovered wrong scale or rotation");
        check(std::abs(controlled.layer.x-expectedX)<1e-4&&std::abs(controlled.layer.y-expectedY)<1e-4,"Control points recovered wrong translation");
        check(controlled.residuals.size()==controlPoints.size()&&controlled.inliers.size()==8&&controlled.inliers.back()==7,"Control-point outlier was not rejected");
        check(controlled.medianError<1e-4&&controlled.maxError<1e-4&&controlled.residuals.back()>500,"Control-point residual diagnostics incorrect");
        rejects([&]{orbyx::alignFromControlPoints(moving,reference,{{{0,0},{0,0}},{{1,1},{1,1}}});});
        rejects([&]{orbyx::alignFromControlPoints(moving,reference,{{{1,1},{0,0}},{{1,1},{2,2}},{{1,1},{3,3}}});});
        cv::Mat bordered(180,240,CV_16UC1,cv::Scalar(1200));cv::Mat textured(120,170,CV_16UC1);cv::randu(textured,cv::Scalar(4000),cv::Scalar(50000));textured.copyTo(bordered(cv::Rect(30,35,170,120)));
        auto borderedFile=root/"bordered.png";cv::imwrite(borderedFile.string(),bordered);orbyx::MosaicLayer borderedLayer;borderedLayer.image=borderedFile;
        auto crop=orbyx::proposeUniformBorderCrop(borderedLayer);check(crop.original==cv::Rect(0,0,240,180)&&crop.proposed.x<=30&&crop.proposed.x>=27&&crop.proposed.y<=35&&crop.proposed.y>=32&&crop.proposed.br().x>=200&&crop.proposed.br().x<=203&&crop.proposed.br().y>=155&&crop.proposed.br().y<=158,"Uniform-border crop proposal missed known content");
        check(crop.retainedFraction>.45&&crop.retainedFraction<.65,"Uniform-border retained fraction incorrect");borderedLayer.crop={20,20,200,150};auto nestedCrop=orbyx::proposeUniformBorderCrop(borderedLayer);check(nestedCrop.original==borderedLayer.crop&&nestedCrop.proposed.x>=borderedLayer.crop.x&&nestedCrop.proposed.y>=borderedLayer.crop.y,"Crop proposal ignored an existing crop");
        orbyx::MosaicLayer uniform;uniform.image=blankFile;rejects([&]{orbyx::proposeUniformBorderCrop(uniform);});
        fs::remove_all(root);std::cout<<"Mosaic geometry, colour, precision, persistence and rejection tests passed\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';fs::remove_all(root);return 1;}
}
