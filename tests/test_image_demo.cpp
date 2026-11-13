// SPDX-License-Identifier: GPL-3.0-only
// Exercise the actual JPEG adapter in headless and ASan/UBSan builds too.
#include "orbyx/engine.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <json/json.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <stdexcept>
namespace fs=std::filesystem;
void check(bool ok,const char *why) {if(!ok)throw std::runtime_error(why);}
std::vector<char> bytes(const fs::path& file) {
    std::ifstream stream(file,std::ios::binary);check(bool(stream),"Input unreadable");
    return {std::istreambuf_iterator<char>(stream),{}};
}
void write(const fs::path& file,const std::vector<char>& value) {
    std::ofstream stream(file,std::ios::binary);stream.write(value.data(),value.size());check(bool(stream),"Write failed");
}
int main(int argc,char **argv) {
    fs::path root;
    try {
        check(argc==2,"Expected source sample directory");
        auto pattern=(fs::temp_directory_path()/"orbyx-jpeg-tests-XXXXXX").string();
        std::vector<char> buffer(pattern.begin(),pattern.end());buffer.push_back(0);
        auto *temporary=::mkdtemp(buffer.data());check(temporary,"Private test directory unavailable");root=temporary;
        const fs::path input=argv[1],output=root/"result",bad=root/"bad",failed=root/"failed";
        std::vector<fs::path> names;
        std::vector<std::vector<char>> originals;
        for(const auto *side:{"sx","dx"})for(int c=1;c<=3;++c) {
            names.emplace_back(std::string(side)+"_MSUGS-VIS-"+std::to_string(c)+".jpg");
            originals.push_back(bytes(input/names.back()));
        }
        orbyx::runImageDemo(input,output);
        for(size_t i=0;i<names.size();++i)check(bytes(input/names[i])==originals[i],"Demo modified a source image");
        const auto raw=cv::imread((output/"composite-raw.png").string(),cv::IMREAD_UNCHANGED);
        const auto display=cv::imread((output/"composite.png").string(),cv::IMREAD_UNCHANGED);
        check(raw.size()==cv::Size(1428,1428)&&raw.type()==CV_8UC3,"Wrong composite dimensions or depth");
        check(display.size()==raw.size()&&display.type()==raw.type()&&cv::norm(raw,display,cv::NORM_INF)>0,"Display product absent");
        // Reference channel 1 is copied after removing the 80-row calibration header.
        for(const auto *side:{"sx","dx"}) {
            const auto original=cv::imread((input/(std::string(side)+"_MSUGS-VIS-1.jpg")).string(),cv::IMREAD_GRAYSCALE);
            const auto aligned=cv::imread((output/(std::string(side)+"-ch1.png")).string(),cv::IMREAD_UNCHANGED);
            check(aligned.type()==CV_8UC1&&aligned.size()==cv::Size(714,1968)&&cv::norm(original(cv::Rect(0,80,714,1968)),aligned,cv::NORM_INF)==0,"Reference channel changed");
        }
        Json::Value report;std::ifstream reportFile(output/"demo-report.json");reportFile>>report;
        for(const auto *side:{"sx","dx"})for(const auto *channel:{"2","3"})
            check(report["alignment"][side][channel]["ecc"].asDouble()>.94,"Registration below reference threshold");
        check(report["seam"]["right_down_pixels"].asInt()==4,"Wrong measured seam shift");
        // Non-bundled resolution: the same imagery resized to a different size must
        // run through the generic path and yield a full 2W x H composite with the
        // channels kept whole (no fixed 80-row header removed, no disk crop).
        {
            const fs::path gin=root/"generic",gout=root/"generic-out";
            fs::create_directory(gin);
            const cv::Size gs(500,1400);
            for(size_t i=0;i<names.size();++i) {
                // Exclude the 80-row calibration ramp first, then resize: a user's own
                // images would not carry that ramp, so the fixture must not either.
                const cv::Mat body=cv::imread((input/names[i]).string(),cv::IMREAD_GRAYSCALE)(cv::Rect(0,80,714,1968));
                cv::Mat small;cv::resize(body,small,gs,0,0,cv::INTER_AREA);
                check(cv::imwrite((gin/names[i]).string(),small,{cv::IMWRITE_JPEG_QUALITY,95}),"Could not encode generic JPEG");
            }
            orbyx::runImageDemo(gin,gout);
            const auto graw=cv::imread((gout/"composite-raw.png").string(),cv::IMREAD_UNCHANGED);
            check(graw.type()==CV_8UC3&&graw.size()==cv::Size(2*gs.width,gs.height),"Generic composite has wrong size or depth");
            for(const auto *side:{"sx","dx"}) {
                const auto ch=cv::imread((gout/(std::string(side)+"-ch1.png")).string(),cv::IMREAD_UNCHANGED);
                check(ch.type()==CV_8UC1&&ch.size()==gs,"Generic channel not kept whole at input size");
            }
            Json::Value greport;std::ifstream gf(gout/"demo-report.json");gf>>greport;
            check(!greport["geometry"]["bundled_sample"].asBool()&&greport["geometry"]["output_width"].asInt()==2*gs.width,"Generic report geometry wrong");
        }
        // Large resolution (>2048 px long side): exercises the bounded-estimate path
        // (ECC and seam measured on a reduced image, then applied at full resolution)
        // and confirms the final composite is produced at full 2W x H size.
        {
            const fs::path lin=root/"large",lout=root/"large-out";
            fs::create_directory(lin);
            const cv::Size ls(1400,3000);
            for(size_t i=0;i<names.size();++i) {
                const cv::Mat body=cv::imread((input/names[i]).string(),cv::IMREAD_GRAYSCALE)(cv::Rect(0,80,714,1968));
                cv::Mat big;cv::resize(body,big,ls,0,0,cv::INTER_LINEAR);
                check(cv::imwrite((lin/names[i]).string(),big,{cv::IMWRITE_JPEG_QUALITY,95}),"Could not encode large JPEG");
            }
            orbyx::runImageDemo(lin,lout);
            const auto lraw=cv::imread((lout/"composite-raw.png").string(),cv::IMREAD_UNCHANGED);
            check(lraw.type()==CV_8UC3&&lraw.size()==cv::Size(2*ls.width,ls.height),"Large composite has wrong size or depth at full resolution");
        }
        // Export enlargement: scale 2 produces a 2x larger composite (interpolated).
        {
            const fs::path sout=root/"scaled-out";
            orbyx::runImageDemo(input,sout,{},2);
            const auto sraw=cv::imread((sout/"composite-raw.png").string(),cv::IMREAD_UNCHANGED);
            check(sraw.type()==CV_8UC3&&sraw.size()==cv::Size(2856,2856),"Export scale did not enlarge the composite");
            Json::Value sreport;std::ifstream sf(sout/"demo-report.json");sf>>sreport;
            check(sreport["display"]["export_scale"].asInt()==2,"Export scale not recorded");
        }
        const auto reject=[&](const fs::path& source,const fs::path& destination) {
            try {orbyx::runImageDemo(source,destination);}catch(const std::exception&){return true;}return false;
        };
        const auto sentinel=bytes(output/"composite.png");
        check(reject(input,output)&&bytes(output/"composite.png")==sentinel,"Existing result overwritten");
        fs::create_directory(bad);
        for(size_t i=0;i<names.size();++i)write(bad/names[i],originals[i]);
        for(size_t length:{0,1,2,3,4,16,32,64,128,256}) {
            write(bad/names[0],{originals[0].begin(),originals[0].begin()+length});
            check(reject(bad,failed)&&!fs::exists(failed),"Truncated JPEG accepted or rollback incomplete");
        }
        // A run of fill markers reaches the exact header boundary that static
        // analysis highlighted. It must throw before accessing another byte.
        write(bad/names[0],{char(255),char(216),char(255),char(255),char(255),char(255)});
        check(reject(bad,failed)&&!fs::exists(failed),"Incomplete marker accepted");
        write(bad/names[0],std::vector<char>(40*1024*1024+1,'x'));
        check(reject(bad,failed)&&!fs::exists(failed),"Oversized JPEG accepted");
        fs::remove(bad/names[0]);fs::create_symlink(fs::absolute(input/names[0]),bad/names[0]);
        check(reject(bad,failed)&&!fs::exists(failed),"Symlink input accepted");
        fs::remove(bad/names[0]);
        check(reject(bad,failed)&&!fs::exists(failed),"Missing channel accepted");
        std::cout<<"Real six-JPEG composition, generic-resolution path, source preservation, measured seam, truncation bounds, size limits, symlink rejection and rollback passed\n";
        fs::remove_all(root);return 0;
    }catch(const std::exception& error) {
        if(!root.empty()){std::error_code ignored;fs::remove_all(root,ignored);}
        std::cerr<<error.what()<<'\n';return 1;
    }
}
