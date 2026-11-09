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
        write(bad/names[0],std::vector<char>(1024*1024+1,'x'));
        check(reject(bad,failed)&&!fs::exists(failed),"Oversized JPEG accepted");
        fs::remove(bad/names[0]);fs::create_symlink(fs::absolute(input/names[0]),bad/names[0]);
        check(reject(bad,failed)&&!fs::exists(failed),"Symlink input accepted");
        fs::remove(bad/names[0]);
        check(reject(bad,failed)&&!fs::exists(failed),"Missing channel accepted");
        std::cout<<"Real six-JPEG composition, source preservation, measured seam, truncation bounds, size limits, symlink rejection and rollback passed\n";
        fs::remove_all(root);return 0;
    }catch(const std::exception& error) {
        if(!root.empty()){std::error_code ignored;fs::remove_all(root,ignored);}
        std::cerr<<error.what()<<'\n';return 1;
    }
}
