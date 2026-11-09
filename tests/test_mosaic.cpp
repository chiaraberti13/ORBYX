// SPDX-License-Identifier: GPL-3.0-only
#include "orbyx/compositor.hpp"
#include <opencv2/imgcodecs.hpp>
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
        fs::remove_all(root);std::cout<<"Mosaic geometry, colour, precision, persistence and rejection tests passed\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';fs::remove_all(root);return 1;}
}
