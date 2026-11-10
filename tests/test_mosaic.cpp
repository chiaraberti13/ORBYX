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
        m.blend=orbyx::MosaicBlend::Replace;m.opacity=.2;m.feather=4;check(cv::norm(orbyx::renderMosaic({l,m}).image,b,cv::NORM_INF)==0,"No-blending mode changed upper-layer values");
        m.blend=orbyx::MosaicBlend::Feather;
        m.opacity=1;m.feather=4;
        auto feather=orbyx::renderMosaic({l,m}).image;
        check(feather.at<cv::Vec3w>(0,0)[0]<feather.at<cv::Vec3w>(3,3)[0],"Feather not applied at boundary");
        cv::Mat seamLower(64,96,CV_16UC3,cv::Scalar(9001,12001,15001)),seamUpper(64,96,CV_16UC3,cv::Scalar(31001,34001,37001));
        check(cv::imwrite((root/"seam-lower.png").string(),seamLower)&&cv::imwrite((root/"seam-upper.png").string(),seamUpper),"Write seam fixtures");
        orbyx::MosaicLayer seamA;seamA.image=root/"seam-lower.png";seamA.blend=orbyx::MosaicBlend::Replace;
        orbyx::MosaicLayer seamB;seamB.image=root/"seam-upper.png";seamB.x=40;seamB.blend=orbyx::MosaicBlend::Seam;
        const auto seamRendered=orbyx::renderMosaic({seamA,seamB});const auto seamResult=seamRendered.image,seamAgain=orbyx::renderMosaic({seamA,seamB}).image;
        check(seamRendered.seamFallbacks==0&&seamResult.type()==CV_16UC3&&seamResult.size()==cv::Size(136,64)&&cv::norm(seamResult,seamAgain,cv::NORM_INF)==0,"Minimum-cost seam is not native-depth deterministic");
        check(seamResult.at<cv::Vec3w>(30,10)==seamLower.at<cv::Vec3w>(30,10)&&seamResult.at<cv::Vec3w>(30,120)==seamUpper.at<cv::Vec3w>(30,80),"Minimum-cost seam lost non-overlap content");
        for(int y=0;y<seamResult.rows;++y)for(int x=0;x<seamResult.cols;++x){const auto pixel=seamResult.at<cv::Vec3w>(y,x);check(pixel==seamLower.at<cv::Vec3w>(0,0)||pixel==seamUpper.at<cv::Vec3w>(0,0),"Minimum-cost seam introduced a hole or unrequested blend");}
        const auto seamProject=orbyx::mosaicProject({seamA,seamB});const auto seamLoaded=orbyx::parseMosaicProject(seamProject);
        check(seamLoaded[1].blend==orbyx::MosaicBlend::Seam&&cv::norm(orbyx::renderMosaic(seamLoaded).image,seamResult,cv::NORM_INF)==0,"Minimum-cost seam did not round trip");
        seamB.feather=8;const auto softenedSeam=orbyx::renderMosaic({seamA,seamB}).image;bool transitionPixel=false;for(int y=0;y<softenedSeam.rows&&!transitionPixel;++y)for(int x=40;x<96;++x){const auto pixel=softenedSeam.at<cv::Vec3w>(y,x);if(pixel!=seamLower.at<cv::Vec3w>(0,0)&&pixel!=seamUpper.at<cv::Vec3w>(0,0)){transitionPixel=true;break;}}check(transitionPixel&&cv::countNonZero(softenedSeam.reshape(1)==0)==0,"Adjustable seam transition was not applied or introduced holes");
        // Multiband blend: a visual, native-depth Laplacian-pyramid blend. It is
        // deterministic, leaves no holes, preserves the non-overlapping lower-only
        // region untouched and produces a real transition across the seam.
        orbyx::MosaicLayer bandA=seamA;orbyx::MosaicLayer bandB;bandB.image=root/"seam-upper.png";bandB.x=40;bandB.blend=orbyx::MosaicBlend::Multiband;
        const auto bandResult=orbyx::renderMosaic({bandA,bandB}).image,bandAgain=orbyx::renderMosaic({bandA,bandB}).image;
        check(bandResult.type()==CV_16UC3&&bandResult.size()==cv::Size(136,64)&&cv::norm(bandResult,bandAgain,cv::NORM_INF)==0,"Multiband blend is not native-depth deterministic");
        check(cv::countNonZero(bandResult.reshape(1)==0)==0,"Multiband blend left holes");
        check(bandResult.at<cv::Vec3w>(30,10)==seamLower.at<cv::Vec3w>(0,0),"Multiband altered the lower-only region");
        bool banded=false;for(int y=0;y<bandResult.rows&&!banded;++y)for(int x=40;x<96;++x){const auto pixel=bandResult.at<cv::Vec3w>(y,x);if(pixel!=seamLower.at<cv::Vec3w>(0,0)&&pixel!=seamUpper.at<cv::Vec3w>(0,0)){banded=true;break;}}
        check(banded,"Multiband blend produced no transition across the seam");
        const auto bandProject=orbyx::mosaicProject({bandA,bandB});const auto bandLoaded=orbyx::parseMosaicProject(bandProject);
        check(bandProject.find("multiband")!=std::string::npos&&bandLoaded[1].blend==orbyx::MosaicBlend::Multiband&&cv::norm(orbyx::renderMosaic(bandLoaded).image,bandResult,cv::NORM_INF)==0,"Multiband blend did not round trip");
        cv::Mat colourReference(72,96,CV_16UC3,cv::Scalar(20000,24000,28000)),colourMoving(72,96,CV_16UC3,cv::Scalar(10000,16000,14000));
        check(cv::imwrite((root/"colour-reference.png").string(),colourReference)&&cv::imwrite((root/"colour-moving.png").string(),colourMoving),"Write compensation fixtures");
        orbyx::MosaicLayer colourA;colourA.image=root/"colour-reference.png";orbyx::MosaicLayer colourB;colourB.image=root/"colour-moving.png";colourB.x=32;
        const auto compensation=orbyx::proposeVisualCompensation({colourA,colourB},1,0);
        check(compensation.samples>=64&&compensation.layer.x==colourB.x&&compensation.layer.y==colourB.y&&compensation.exposureDelta>0,"Visual compensation proposal is incomplete or changed geometry");
        const auto corrected=orbyx::renderMosaic({compensation.layer}).image,referencePixels=orbyx::renderMosaic({colourA}).image;
        check(cv::norm(corrected,referencePixels,cv::NORM_INF)<350,"Visual compensation did not robustly match the overlap colour");
        check(cv::norm(cv::imread((root/"colour-moving.png").string(),cv::IMREAD_UNCHANGED),colourMoving,cv::NORM_INF)==0,"Visual compensation altered the source image");
        colourB.x=200;rejects([&]{orbyx::proposeVisualCompensation({colourA,colourB},1,0);});
        m=l;m.x=-5;m.y=-3;auto negative=orbyx::renderMosaic({m});
        check(negative.originX==-5&&negative.originY==-3&&cv::norm(negative.image,a,cv::NORM_INF)==0,"Negative-origin placement incorrect");
        m=l;m.crop={2,1,4,3};check(orbyx::renderMosaic({m}).image.size()==cv::Size(4,3),"Crop failed");
        m.angle=90;auto rotate=orbyx::renderMosaic({m});check(rotate.image.cols<=4&&rotate.image.rows<=5,"Rotation canvas incorrect");
        m=l;m.scale=2;check(orbyx::renderMosaic({m}).image.size()==cv::Size(18,14),"Scale failed");
        m=l;m.saturation=0;auto grey=orbyx::renderMosaic({m}).image.at<cv::Vec3w>(3,3);check(grey[0]==grey[1]&&grey[1]==grey[2],"Saturation failed");
        m=l;m.exposure=1;auto exposure=orbyx::renderMosaic({m}).image.at<cv::Vec3w>(3,3);check(exposure[0]==2050,"Exposure oracle failed");
        m=l;m.visible=false;check(cv::norm(orbyx::renderMosaic({l,m}).image,a,cv::NORM_INF)==0,"Hidden layer rendered");
        n.blend=orbyx::MosaicBlend::Replace;auto project=orbyx::mosaicProject({l,n});auto loaded=orbyx::parseMosaicProject(project);check(loaded[1].blend==orbyx::MosaicBlend::Replace,"Project lost blend mode");
        auto legacyProject=project;for(auto position=legacyProject.find("\"blend\"");position!=std::string::npos;position=legacyProject.find("\"blend\"")){const auto start=legacyProject.rfind('\n',position),end=legacyProject.find('\n',position);check(start!=std::string::npos&&end!=std::string::npos,"Cannot construct legacy project fixture");legacyProject.erase(start+1,end-start);}auto legacyLoaded=orbyx::parseMosaicProject(legacyProject);check(legacyLoaded[0].blend==orbyx::MosaicBlend::Feather&&legacyLoaded[1].blend==orbyx::MosaicBlend::Feather,"Legacy project did not retain feather semantics");
        check(cv::norm(orbyx::renderMosaic(loaded).image,orbyx::renderMosaic({l,n}).image,cv::NORM_INF)==0,"Project round trip changed pixels");
        // M2-13: portable project — images under the project folder are stored
        // relative and resolve against the folder, so the project moves with them.
        {
            const auto proj=root/"project-a";fs::create_directories(proj);
            cv::imwrite((proj/"layer.png").string(),cv::Mat(8,8,CV_16UC3,cv::Scalar(100,200,300)));
            orbyx::MosaicLayer pl;pl.image=proj/"layer.png";
            const auto portable=orbyx::mosaicProject({pl},proj);
            check(portable.find("\"layer.png\"")!=std::string::npos&&portable.find(proj.string())==std::string::npos,"Portable project did not store a relative image path");
            const auto moved=root/"project-b";fs::create_directories(moved);cv::imwrite((moved/"layer.png").string(),cv::Mat(8,8,CV_16UC3,cv::Scalar(100,200,300)));
            const auto reopened=orbyx::parseMosaicProject(portable,moved);
            check(reopened.size()==1&&reopened[0].image==(moved/"layer.png").lexically_normal(),"Portable project did not resolve relative to the moved folder");
            check(orbyx::mosaicProject({pl})==orbyx::mosaicProject({pl},fs::path()),"Absolute project serialization changed");
            rejects([&]{orbyx::parseMosaicProject(portable,fs::path());}); // relative path needs a base folder
            auto escaped=portable;const auto pos=escaped.find("\"layer.png\"");escaped.replace(pos,11,"\"../escape.png\"");
            rejects([&]{orbyx::parseMosaicProject(escaped,moved);}); // escaping the project folder is rejected
        }
        // M2-11: the export report declares the bit depth and colour note, embeds the
        // reproducible project, is deterministic and records the exact depth.
        const auto report16=orbyx::mosaicReport({l,n},"mosaic.png",16);
        check(report16==orbyx::mosaicReport({l,n},"mosaic.png",16),"Export report is not deterministic");
        check(report16.find("orbyx-mosaic-report/1")!=std::string::npos&&report16.find("orbyx-mosaic/1")!=std::string::npos,"Export report omits the schema or the embedded project");
        check(report16.find("bit_depth")!=std::string::npos&&report16.find("radiometr")!=std::string::npos,"Export report omits the bit depth or colour note");
        check(orbyx::mosaicReport({l,n},"mosaic.png",8)!=report16,"Export report ignores the bit depth");
        rejects([&]{orbyx::mosaicReport({l,n},"mosaic.png",10);});
        // M2-11: the report declares the formal colour processing order.
        check(report16.find("\"pipeline\"")!=std::string::npos&&report16.find("one transform per layer")!=std::string::npos&&
              report16.find("compose colour")!=std::string::npos,"Export report omits the colour processing order");
        // M2-11: a colour layer is warped with one transform for every colour channel,
        // so an integer translation moves R/G/B together and the canvas-cropped result
        // is identical to the source (no per-channel divergence or coloured fringe).
        {
            orbyx::MosaicLayer shifted=n;shifted.x=4;shifted.y=3;const auto moved=orbyx::renderMosaic({shifted}).image;
            const auto source=cv::imread((root/"c.png").string(),cv::IMREAD_UNCHANGED);
            check(moved.size()==source.size()&&cv::norm(moved,source,cv::NORM_INF)==0,"Colour channels diverged under a single-layer transform");
        }
        // M2-12: quality indicators (evidence only; deterministic; never "perfect").
        {
            orbyx::MosaicLayer qa;qa.image=root/"seam-lower.png";qa.blend=orbyx::MosaicBlend::Replace;
            const auto single=orbyx::mosaicQuality({qa});
            check(single.coveredPixels>0&&single.emptyFraction<0.02&&single.overlapFraction==0.&&single.extremeTransforms==0,"Quality single-layer mismeasured");
            orbyx::MosaicLayer qb;qb.image=root/"seam-upper.png";qb.x=40;qb.blend=orbyx::MosaicBlend::Replace;
            check(orbyx::mosaicQuality({qa,qb}).overlapFraction>0.05,"Quality did not detect overlap");
            orbyx::MosaicLayer qfar=qb;qfar.x=300;const auto gapped=orbyx::mosaicQuality({qa,qfar});
            check(gapped.overlapFraction==0.&&gapped.emptyFraction>0.1,"Quality did not detect an empty gap");
            orbyx::MosaicLayer bright=qa;bright.exposure=10;check(orbyx::mosaicQuality({bright}).highlightClipped>0.5,"Quality did not detect highlight clipping");
            orbyx::MosaicLayer rotated=qa;rotated.angle=90;check(orbyx::mosaicQuality({rotated}).extremeTransforms==1,"Quality did not flag an extreme transform");
            const auto d1=orbyx::mosaicQuality({qa,qb}),d2=orbyx::mosaicQuality({qa,qb});
            check(d1.coveredPixels==d2.coveredPixels&&d1.overlapFraction==d2.overlapFraction&&d1.emptyFraction==d2.emptyFraction,"Quality assessment is not deterministic");
            // Seam difference: different overlapping layers disagree strongly; identical
            // ones agree (only a thin boundary residual remains); it is deterministic.
            check(orbyx::mosaicQuality({qa,qb}).seamDifference>0.2&&d1.seamDifference==d2.seamDifference,"Quality did not measure (deterministically) a seam difference");
            orbyx::MosaicLayer qdup;qdup.image=root/"seam-lower.png";qdup.x=40;qdup.blend=orbyx::MosaicBlend::Replace;
            check(orbyx::mosaicQuality({qa,qdup}).seamDifference<0.05,"Identical overlapping layers should have a much lower seam difference");
        }
        // Non-destructive exclude mask: the masked region of the upper layer does
        // not contribute, revealing the layer below; outside it the upper layer is
        // unchanged and the source file is never modified.
        m=l;m.image=root/"b.png";m.blend=orbyx::MosaicBlend::Replace;m.exclude={2,1,4,3};
        auto masked=orbyx::renderMosaic({l,m}).image;
        check(cv::norm(masked(cv::Rect(2,1,4,3)),a(cv::Rect(2,1,4,3)),cv::NORM_INF)==0,"Exclude mask did not reveal the lower layer");
        check(cv::norm(masked(cv::Rect(0,0,2,7)),b(cv::Rect(0,0,2,7)),cv::NORM_INF)==0,"Exclude mask leaked outside its region");
        check(cv::norm(cv::imread((root/"b.png").string(),cv::IMREAD_UNCHANGED),b,cv::NORM_INF)==0,"Exclude mask altered the source image");
        auto maskProject=orbyx::mosaicProject({l,m});check(maskProject.find("\"exclude\"")!=std::string::npos,"Exclude mask was not serialized");
        auto maskLoaded=orbyx::parseMosaicProject(maskProject);check(maskLoaded[1].exclude==cv::Rect(2,1,4,3),"Project lost the exclude mask");
        check(cv::norm(orbyx::renderMosaic(maskLoaded).image,masked,cv::NORM_INF)==0,"Exclude mask round trip changed pixels");
        check(orbyx::mosaicProject({l}).find("\"exclude\"")==std::string::npos,"Empty exclude mask must not be serialized");
        m=l;m.image=root/"b.png";m.exclude={8,0,5,2};rejects([&]{orbyx::renderMosaic({l,m});});
        m=l;m.exclude={0,0,-4,5};rejects([&]{orbyx::renderMosaic({m});});
        m=l;m.exclude={0,0,200000,5};rejects([&]{orbyx::renderMosaic({m});});
        // A zero width or height is "no mask", not an error, and never serialized.
        m=l;m.exclude={10,10,4,0};check(cv::norm(orbyx::renderMosaic({m}).image,a,cv::NORM_INF)==0,"Zero-height exclude unexpectedly masked");
        check(orbyx::mosaicProject({m}).find("\"exclude\"")==std::string::npos,"Inactive exclude mask must not be serialized");
        // Polygon masks use cropped-layer coordinates. Include polygons form a
        // union, while exclusions override them; sources remain unchanged.
        m=l;m.image=root/"b.png";m.blend=orbyx::MosaicBlend::Replace;m.masks={orbyx::MosaicMask{false,{{2,1},{6,1},{2,5}}}};
        auto polygonExcluded=orbyx::renderMosaic({l,m}).image;
        check(polygonExcluded.at<cv::Vec3w>(2,3)==a.at<cv::Vec3w>(2,3)&&polygonExcluded.at<cv::Vec3w>(6,8)==b.at<cv::Vec3w>(6,8),"Polygon exclude mask did not reveal only the lower layer");
        m.masks={orbyx::MosaicMask{true,{{0,0},{5,0},{0,5}}}};auto polygonIncluded=orbyx::renderMosaic({l,m}).image;
        check(polygonIncluded.at<cv::Vec3w>(1,1)==b.at<cv::Vec3w>(1,1)&&polygonIncluded.at<cv::Vec3w>(6,8)==a.at<cv::Vec3w>(6,8),"Polygon include mask did not limit the upper layer");
        m.masks.push_back(orbyx::MosaicMask{false,{{0,0},{3,0},{0,3}}});auto polygonCombined=orbyx::renderMosaic({l,m}).image;
        check(polygonCombined.at<cv::Vec3w>(1,1)==a.at<cv::Vec3w>(1,1),"Polygon exclusion did not override inclusion");
        auto polygonProject=orbyx::mosaicProject({l,m});auto polygonLoaded=orbyx::parseMosaicProject(polygonProject);
        check(polygonLoaded[1].masks.size()==2&&polygonLoaded[1].masks[0].include&& !polygonLoaded[1].masks[1].include&&cv::norm(orbyx::renderMosaic(polygonLoaded).image,polygonCombined,cv::NORM_INF)==0,"Polygon masks did not round trip");
        check(cv::norm(cv::imread((root/"b.png").string(),cv::IMREAD_UNCHANGED),b,cv::NORM_INF)==0,"Polygon masks altered the source image");
        m.masks={orbyx::MosaicMask{false,{{0,0},{1,1}}}};rejects([&]{orbyx::mosaicProject({m});});
        m.masks={orbyx::MosaicMask{false,{{0,0},{20,0},{0,5}}}};rejects([&]{orbyx::renderMosaic({m});});
        rejects([&]{orbyx::parseMosaicProject(project+"{}");});
        rejects([&]{orbyx::parseMosaicProject("{\"schema\":\"orbyx-mosaic/1\",\"schema\":\"bad\",\"layers\":[]}");});
        auto invalidBlend=project;auto blendValue=invalidBlend.find("replace");check(blendValue!=std::string::npos,"Blend field missing");invalidBlend.replace(blendValue,7,"invalid");rejects([&]{orbyx::parseMosaicProject(invalidBlend);});
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
        // M2-12 quantitative validation on known truth: a correctly aligned textured
        // pair coregisters in the overlap (low seam difference); a deliberate
        // misalignment scores clearly higher; a far portion is a no-overlap gap.
        {
            const orbyx::MosaicLayer A=automatic[0],B=arranged.layers[1];
            const auto good=orbyx::mosaicQuality({A,B});
            check(good.overlapFraction>0.05&&good.emptyFraction<0.2&&good.seamDifference<0.05,"Known-truth: aligned pair failed the quality criteria");
            orbyx::MosaicLayer bad=B;bad.x+=40;bad.y+=30;const auto poor=orbyx::mosaicQuality({A,bad});
            check(poor.seamDifference>good.seamDifference+0.05,"Known-truth: seam difference did not penalise a 40/30 px misalignment");
            orbyx::MosaicLayer far=automatic[1];far.x=1200;far.y=0;const auto gap=orbyx::mosaicQuality({A,far});
            check(gap.overlapFraction==0.&&gap.emptyFraction>0.1,"Known-truth: no-overlap case misclassified");
        }
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
        check(!controlled.spatiallyConcentrated&&controlled.movingCoverage>.1&&controlled.referenceCoverage>.1,"Well-spread control points reported as concentrated");
        std::vector<orbyx::ControlPointPair> clustered;for(const cv::Point2d point:std::vector<cv::Point2d>{{10,10},{18,10},{10,18},{18,18}})clustered.push_back({point,{point.x+3,point.y-2}});
        auto clusteredSolution=orbyx::alignFromControlPoints(moving,reference,clustered);check(clusteredSolution.spatiallyConcentrated&&clusteredSolution.movingCoverage<.01&&clusteredSolution.referenceCoverage<.01,"Concentrated control points did not produce a warning");
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
