// GPL-3.0. Affine/ECC workflow derived from RDAS Layer Aligner by Digitelektro.
#include "orbyx/engine.hpp"
#include "orbyx/compositor.hpp"
#include <opencv2/core/ocl.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <json/json.h>
#include <array>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
#include <cmath>
#include <fstream>
#include <future>
#include <limits>
#include <random>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#ifdef __linux__
#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(__APPLE__)
#include <stdio.h>
#endif
namespace orbyx {
namespace {
namespace fs = std::filesystem;
constexpr int ch2offset = 3606, ch3offset = 1804;
using Channels = std::array<cv::Mat, 3>;
void require(bool condition, const std::string& message) {
  if (!condition) throw std::runtime_error(message);
}
void regular(const fs::path& file) {
  require(!fs::is_symlink(fs::symlink_status(file)) && fs::is_regular_file(file),
          "Expected a regular, non-symlink file: " + file.string());
}
// Decode/parse exactly the bounded bytes that were checked, never reopen a path
// after the preflight. Concurrent edits may invalidate the snapshot, but cannot
// replace the decoder input with an unchecked larger file.
std::vector<unsigned char> snapshot(const fs::path& file, std::uintmax_t limit,
                                    const char* limitError) {
  regular(file);
  const auto size=fs::file_size(file);
  require(size<=limit,limitError);
  std::vector<unsigned char> bytes(static_cast<std::size_t>(size));
  std::ifstream stream(file,std::ios::binary);
  require(bool(stream),"Cannot read input file: "+file.string());
  if(size) stream.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(size));
  require((!size || static_cast<std::uintmax_t>(stream.gcount())==size) &&
          stream.peek()==std::char_traits<char>::eof() && !stream.bad(),
          "Input file changed or could not be read: "+file.string());
  return bytes;
}
Json::Value readConfig(const fs::path& file) {
  const auto bytes=snapshot(file,1024*1024,"Calibration profile exceeds 1 MiB");
  std::istringstream stream(std::string(bytes.begin(),bytes.end()));
  Json::CharReaderBuilder reader;
  reader["rejectDupKeys"] = true;
  reader["failIfExtra"] = true;
  Json::Value root;
  std::string errors;
  require(Json::parseFromStream(reader, stream, &root, &errors) && root.isObject(),
          "Invalid calibration JSON: " + errors);
  return root;
}
cv::Mat transform(const Json::Value& section, const char* channel) {
  const auto& value = section[channel];
  require(value.isObject(), std::string("Missing transform: ") + channel);
  constexpr std::array<const char*, 6> keys{"a", "b", "tx", "c", "d", "ty"};
  cv::Mat matrix(2, 3, CV_32F);
  for (int i = 0; i < 6; ++i) {
    const auto& v = value[keys[i]];
    require(v.isNumeric(), std::string("Missing numeric coefficient: ") + keys[i]);
    const double number = v.asDouble();
    require(std::isfinite(number) && std::abs(number) <= 1e7,
            "Non-finite or excessive affine coefficient");
    matrix.at<float>(i / 3, i % 3) = static_cast<float>(number);
  }
  const double determinant = matrix.at<float>(0, 0) * double(matrix.at<float>(1, 1)) -
                             matrix.at<float>(0, 1) * double(matrix.at<float>(1, 0));
  require(std::abs(determinant) > 1e-8, "Singular affine transform");
  return matrix;
}
cv::Rect roi(const Json::Value& section, const cv::Size& size) {
  const auto& value = section["ROI"];
  require(value.isObject(), "ROI must be an object");
  for (const auto* key : {"x", "y", "width", "height"})
    require(value[key].isInt(), std::string("Invalid ROI integer: ") + key);
  const int x=value["x"].asInt(), y=value["y"].asInt();
  const int w=value["width"].asInt(), h=value["height"].asInt();
  require(x >= 0 && y >= 0 && w > 0 && h > 0 && x <= size.width && y <= size.height &&
          w <= size.width - x && h <= size.height - y, "ROI outside aligned image");
  return {x,y,w,h};
}
cv::Mat readImage(const fs::path& file, bool rdas=true, bool allowRgb=false) {
  // Bound allocations from the same immutable PNG bytes passed to the decoder.
  // rdas=false relaxes the raw-channel height rule for already-cropped aligned
  // products (composition inputs), keeping every other bound and format check.
  const auto bytes=snapshot(file,1024ULL*1024*1024,"PNG file exceeds 1 GiB");
  require(bytes.size()>=33,"Invalid PNG signature or IHDR");
  std::array<unsigned char,33> header{};
  std::copy_n(bytes.begin(),header.size(),header.begin());
  constexpr std::array<unsigned char,8> signature{137,80,78,71,13,10,26,10};
  require(std::equal(signature.begin(),signature.end(),header.begin()) &&
          header[8]==0&&header[9]==0&&header[10]==0&&header[11]==13 &&
          header[12]=='I'&&header[13]=='H'&&header[14]=='D'&&header[15]=='R',
          "Invalid PNG signature or IHDR");
  auto integer=[&](int i){return (std::uint32_t(header[i])<<24)|(std::uint32_t(header[i+1])<<16)|
                              (std::uint32_t(header[i+2])<<8)|header[i+3];};
  const auto width=integer(16),height=integer(20);
  require(width>0&&height>(rdas?std::uint32_t(ch2offset):0u)&&width<=100000&&height<=100000&&
          std::uint64_t(width)*height<=150000000, "PNG dimensions exceed RDAS safety limits");
  require((header[24]==8||header[24]==16)&&(header[25]==0||(allowRgb&&header[25]==2)) && header[26]==0&&header[27]==0&&header[28]<=1,
          "Expected an unsigned 8-bit or 16-bit grayscale PNG");
  cv::Mat image = cv::imdecode(bytes, cv::IMREAD_UNCHANGED);
  require(!image.empty(), "Cannot decode image: " + file.string());
  require((image.channels()==1||(allowRgb&&image.channels()==3)) && (image.depth()==CV_8U || image.depth()==CV_16U),
          "Expected an unsigned 8-bit or 16-bit grayscale channel");
  require(image.cols==static_cast<int>(width) && image.rows==static_cast<int>(height),
          "Decoded PNG dimensions differ from IHDR");
  require((!rdas || image.rows > ch2offset) && image.cols > 0 && image.total() <= 150000000,
          "Invalid RDAS channel dimensions (height must exceed 3606; max 150 Mpx)");
  return image;
}
Channels load(const fs::path& directory,bool rdas=true) {
  require(!fs::is_symlink(fs::symlink_status(directory)) && fs::is_directory(directory),
          "Expected non-symlink MSUGS_VIS directory");
  Channels channels;
  for (int i=0;i<3;++i) {
    channels[i] = readImage(directory / ("MSUGS-VIS-"+std::to_string(i+1)+".png"),rdas);
    if (i) require(channels[i].size()==channels[0].size() && channels[i].type()==channels[0].type(),
                   "All channels in a VIS product must have equal dimensions and depth");
  }
  return channels;
}
cv::Mat ecc(const cv::Mat& reference, const cv::Mat& target, int offset, double scale) {
  // Preserve upstream: grayscale conversion, pre-shift, affine ECC 500/1e-7.
  cv::Mat ref8, target8;
  if (reference.depth() == CV_16U) {
    reference.convertTo(ref8,CV_8U,1.0/256.0);
    target.convertTo(target8,CV_8U,1.0/256.0);
  } else { ref8=reference; target8=target; }
  if(scale<1.0) {
    const cv::Size size(std::max(1,cvRound(reference.cols*scale)),
                        std::max(1,cvRound(reference.rows*scale)));
    require(size.width>=16 && size.height>=16,"Calibration working image is too small");
    cv::resize(ref8,ref8,size,0,0,cv::INTER_AREA);
    cv::resize(target8,target8,size,0,0,cv::INTER_AREA);
  }
  const double sx=double(ref8.cols)/reference.cols, sy=double(ref8.rows)/reference.rows;
  cv::Mat shifted;
  cv::Mat shift=(cv::Mat_<float>(2,3)<<1,0,0,0,1,offset*sy);
  cv::warpAffine(target8,shifted,shift,target8.size(),cv::INTER_LINEAR|cv::WARP_INVERSE_MAP);
  cv::Mat warp=cv::Mat::eye(2,3,CV_32F);
  const double correlation=cv::findTransformECC(ref8,shifted,warp,cv::MOTION_AFFINE,
        cv::TermCriteria(cv::TermCriteria::COUNT|cv::TermCriteria::EPS,500,1e-7));
  require(std::isfinite(correlation) && cv::checkRange(warp), "ECC did not produce a finite solution");
  // OpenCV resize maps native pixel centres through S, including half-pixel
  // translation. Lift the estimated inverse-map affine with S^-1 * W * S.
  const cv::Matx33d s(sx,0,(sx-1)/2,0,sy,(sy-1)/2,0,0,1);
  cv::Matx33d w= cv::Matx33d::eye();
  for(int row=0;row<2;++row) for(int col=0;col<3;++col) w(row,col)=warp.at<float>(row,col);
  const auto native=s.inv()*w*s;
  for(int row=0;row<2;++row) for(int col=0;col<3;++col) warp.at<float>(row,col)=static_cast<float>(native(row,col));
  warp.at<float>(1,2) += offset;
  return warp;
}
void setTransform(Json::Value& section,const char* channel,const cv::Mat& warp) {
  constexpr std::array<const char*,6> keys{"a","b","tx","c","d","ty"};
  for(int i=0;i<6;++i) section[channel][keys[i]]=warp.at<float>(i/3,i%3);
}
// Some OpenCL GPU drivers evaluate warpAffine differently from the CPU path:
// certain integrated GPUs have been observed to diverge by ~157/255 levels on
// the same INTER_LINEAR|WARP_INVERSE_MAP affine that align() performs. ORBYX's
// alignment must reproduce the verified CPU estimator, so the GPU is trusted
// only when it matches the CPU on this exact machine; otherwise alignment stays
// on the CPU. This selects a backend for correctness; the algorithm is identical.
// The probe mirrors tests/verify_opencl_gpu.cpp and is evaluated once per process.
//   1  = a GPU reproduces the CPU affine within one level (safe to accelerate)
//   0  = no usable OpenCL GPU is present (CPU backend, not an anomaly)
//  -1  = a GPU is present but its affine diverges from the CPU (CPU fallback)
int openclAffineStatus() {
  static const int status=[]{
    try {
      if(!cv::ocl::haveOpenCL()) return 0;
      std::vector<cv::ocl::PlatformInfo> platforms;
      cv::ocl::getPlatfomsInfo(platforms);
      bool gpuPresent=false;
      for(const auto& platform:platforms)
        for(int i=0;i<platform.deviceNumber();++i) {
          cv::ocl::Device device;platform.getDevice(device,i);
          if(device.type()&cv::ocl::Device::TYPE_GPU) gpuPresent=true;
        }
      if(!gpuPresent) return 0;
      cv::ocl::setUseOpenCL(true);
      if(!cv::ocl::useOpenCL()) return 0;
      const cv::Mat transform=(cv::Mat_<float>(2,3)<<1.001,0.002,2.125,-0.001,0.999,1.5);
      for(const int depth:{CV_8U,CV_16U}) {
        cv::Mat probe(257,383,depth);cv::RNG random(981);
        random.fill(probe,cv::RNG::UNIFORM,0,depth==CV_8U?255:65535);
        cv::GaussianBlur(probe,probe,cv::Size(9,9),2);
        cv::Mat cpu,gpu;
        cv::ocl::setUseOpenCL(false);
        cv::warpAffine(probe,cpu,transform,probe.size(),cv::INTER_LINEAR|cv::WARP_INVERSE_MAP);
        cv::ocl::setUseOpenCL(true);
        cv::UMat source,aligned;probe.copyTo(source);
        cv::warpAffine(source,aligned,transform,probe.size(),cv::INTER_LINEAR|cv::WARP_INVERSE_MAP);
        aligned.copyTo(gpu);
        if(cv::norm(cpu,gpu,cv::NORM_INF)>1.0) return -1;
      }
      return 1;
    } catch(const cv::Exception&) {
      return 0; // A missing or raising OpenCL runtime is an absent GPU, not a fault.
    }
  }();
  return status;
}
Channels align(const Channels& input,const Json::Value& section) {
  Channels result;
  result[0]=input[0];
  for (int i=1;i<3;++i) {
    cv::UMat aligned;
    cv::warpAffine(input[i].getUMat(cv::ACCESS_READ),aligned,
                   transform(section,i==1?"CH2":"CH3"),input[i].size(),
                   cv::INTER_LINEAR|cv::WARP_INVERSE_MAP);
    aligned.copyTo(result[i]);
  }
  const cv::Rect crop(0,0,input[0].cols,input[0].rows-ch2offset);
  for(auto& image:result) image=image(crop);
  return result;
}
void writeProduct(const fs::path& directory,const Channels& channels,const fs::path& sourceCbor) {
  fs::create_directory(directory);
  for(int i=0;i<3;++i)
    require(cv::imwrite((directory/("MSUGS-VIS-"+std::to_string(i+1)+".png")).string(),channels[i]),
            "Failed to encode output PNG");
  regular(sourceCbor);
  fs::copy_file(sourceCbor,directory/"product.cbor");
}
void writeSelectedProduct(const fs::path& directory,const Channels& channels,const fs::path& sourceCbor) {
  fs::create_directory(directory);
  for(int i=0;i<3;++i)
    require(cv::imwrite((directory/("MSUGS-VIS-"+std::to_string(i+1)+".png")).string(),channels[i]),
            "Failed to encode output PNG");
  if(fs::exists(fs::symlink_status(sourceCbor))) {
    regular(sourceCbor);
    fs::copy_file(sourceCbor,directory/"product.cbor");
  }
}
class Staging {
 public:
  fs::path path;
  explicit Staging(const fs::path& destination) {
    require(fs::is_directory(destination.parent_path()), "Output parent directory must exist");
    require(!fs::exists(fs::symlink_status(destination)), "Output destination already exists");
    std::random_device random;
    for(int i=0;i<32;++i) {
      path=destination.parent_path()/(".orbyx-"+std::to_string(random())+"-"+std::to_string(random()));
      if(fs::create_directory(path)) {
        fs::permissions(path,fs::perms::owner_all,fs::perm_options::replace);
        return;
      }
    }
    throw std::runtime_error("Cannot create private staging directory");
  }
  ~Staging() { std::error_code ignored; if(!path.empty()) fs::remove_all(path,ignored); }
  void publish(const fs::path& destination) {
    // Recheck immediately before same-filesystem atomic rename. Never replace a product directory.
    require(!fs::exists(fs::symlink_status(destination)), "Output appeared during processing");
#ifdef __linux__
    // Linux renameat2 RENAME_NOREPLACE closes the check/rename race.
    require(::syscall(SYS_renameat2,AT_FDCWD,path.c_str(),AT_FDCWD,destination.c_str(),1)==0,
            std::string("Cannot publish output exclusively: ")+std::strerror(errno));
#elif defined(__APPLE__)
    require(::renamex_np(path.c_str(),destination.c_str(),RENAME_EXCL)==0,
            std::string("Cannot publish output exclusively: ")+std::strerror(errno));
#else
    throw std::runtime_error("Exclusive atomic publication is unsupported on this platform");
#endif
    path.clear();
  }
};
}
std::string translate(const std::string& message,const std::string& language) {
  if(language!="it") return message;
  // Prefixes preserve technical paths and upstream identifiers verbatim.
  static const std::map<std::string,std::string> catalog{
    {"Expected a regular, non-symlink file: ","È richiesto un file normale, non un collegamento simbolico: "},
    {"Calibration profile exceeds 1 MiB","Il profilo di calibrazione supera 1 MiB"},
    {"Cannot read calibration profile","Impossibile leggere il profilo di calibrazione"},
    {"Invalid calibration JSON: ","JSON di calibrazione non valido: "},
    {"Missing transform: ","Trasformazione mancante: "},
    {"Missing numeric coefficient: ","Coefficiente numerico mancante: "},
    {"Non-finite or excessive affine coefficient","Coefficiente affine non finito o eccessivo"},
    {"Singular affine transform","Trasformazione affine singolare"},
    {"ROI must be an object","La ROI deve essere un oggetto"},
    {"Invalid ROI integer: ","Intero ROI non valido: "},
    {"ROI outside aligned image","La ROI esce dai limiti dell'immagine allineata"},
    {"PNG file exceeds 1 GiB","Il file PNG supera 1 GiB"},
    {"Invalid PNG signature or IHDR","Firma PNG o intestazione IHDR non valida"},
    {"PNG dimensions exceed RDAS safety limits","Le dimensioni PNG superano i limiti di sicurezza RDAS"},
    {"Expected an unsigned 8-bit or 16-bit grayscale PNG","È richiesto un PNG in scala di grigi a 8 o 16 bit senza segno"},
    {"Cannot read input file: ","Impossibile leggere il file di input: "},
    {"Input file changed or could not be read: ","Il file di input è cambiato o non è leggibile: "},
    {"Decoded PNG dimensions differ from IHDR","Le dimensioni del PNG decodificato differiscono da IHDR"},
    {"Cannot decode image: ","Impossibile decodificare l'immagine: "},
    {"Expected an unsigned 8-bit or 16-bit grayscale channel","È richiesto un canale in scala di grigi a 8 o 16 bit senza segno"},
    {"Invalid RDAS channel dimensions (height must exceed 3606; max 150 Mpx)","Dimensioni canale RDAS non valide (altezza superiore a 3606; massimo 150 Mpx)"},
    {"Expected non-symlink MSUGS_VIS directory","È richiesta una cartella MSUGS_VIS, non un collegamento simbolico"},
    {"All channels in a VIS product must have equal dimensions and depth","Tutti i canali VIS devono avere dimensioni e profondità uguali"},
    {"ECC did not produce a finite solution","ECC non ha prodotto una soluzione finita"},
    {"Calibration backend: CPU (ECC)","Backend di calibrazione: CPU (ECC)"},
    {"Natural Color output must have a .png extension","L'uscita Natural Color deve avere estensione .png"},
    {"All channels must share dimensions and depth for composition","I canali da comporre devono avere dimensioni e profondità uguali"},
    {"Failed to encode composed RGB product","Impossibile codificare il prodotto RGB composto"},
    {"Calibration scale must be finite and in (0, 1]","La scala di calibrazione deve essere finita e compresa in (0, 1]"},
    {"Calibration scale is only valid in calibrate mode","La scala di calibrazione è valida solo in modalità calibrate"},
    {"Product selection is only valid in generate mode","La selezione dei prodotti è valida solo in modalità generate"},
    {"Too many selected VIS products","Troppi prodotti VIS selezionati"},
    {"Invalid selected VIS product","Prodotto VIS selezionato non valido"},
    {"Duplicate selected VIS product","Prodotto VIS selezionato duplicato"},
    {"Selected VIS products written for the editable compositor","Prodotti VIS selezionati salvati per il compositore modificabile"},
    {"Calibration working image is too small","L'immagine di lavoro per la calibrazione è troppo piccola"},
    {"ECC working scale: ","Scala di lavoro ECC: "},
    {"Failed to encode output PNG","Impossibile codificare il PNG in uscita"},
    {"Output parent directory must exist","La cartella che conterrà l'output deve esistere"},
    {"Output destination already exists","La destinazione di output esiste già"},
    {"Cannot create private staging directory","Impossibile creare la cartella temporanea privata"},
    {"Output appeared during processing","La destinazione è stata creata durante l'elaborazione"},
    {"Cannot publish output exclusively: ","Impossibile pubblicare l'output senza sovrascritture: "},
    {"Exclusive atomic publication is unsupported on this platform","La pubblicazione atomica esclusiva non è supportata su questa piattaforma"},
    {"Specify a satellite/profile identifier","Specificare un identificativo satellite/profilo"},
    {"Invalid satellite identifier","Identificativo satellite non valido"},
    {"Input, output and calibration profile are required","Sono richiesti input, output e profilo di calibrazione"},
    {"Input must be a directory","L'input deve essere una cartella"},
    {"Input and output must differ","Input e output devono essere diversi"},
    {"Source and destination satellites must differ","I satelliti di origine e destinazione devono essere diversi"},
    {"All channels must share dimensions and depth for composition","Tutti i canali devono avere dimensioni e profondità uguali per la composizione"},
    {"Failed to encode composed RGB product","Impossibile codificare il prodotto RGB composto"},
    {"No calibration for ","Calibrazione mancante per "},
    {"Failed to save calibration profile","Impossibile salvare il profilo di calibrazione"},
    {"Calibrated profile; merge ROIs still require manual validation","Profilo calibrato; le ROI di fusione richiedono ancora una verifica manuale"},
    {"VIS products have different bit depths","I prodotti VIS hanno profondità diverse"},
    {"Merged dimensions overflow","Le dimensioni della fusione superano i limiti"},
    {"Writing merged aligned channels","Salvataggio dei canali allineati e uniti"},
    {"No complete merge ROIs; writing separate aligned VIS products","ROI di fusione incomplete; salvataggio dei prodotti VIS allineati separati"},
    {"CBOR copied unchanged; SatDump geometry interoperability requires dataset validation","CBOR copiato senza modifiche; l'interoperabilità geometrica con SatDump richiede la verifica con dataset"},
    {"Backend: OpenCL","Motore: OpenCL"},{"Backend: CPU","Motore: CPU"},
    {"OpenCL GPU affine diverges from CPU; using CPU backend",
     "L'affine OpenCL su GPU diverge dalla CPU; uso del motore CPU"},
    {"Loading ","Caricamento "},{"Calibrating ","Calibrazione "},{"Completed: ","Completato: "},
    {"Mode specified more than once","Modalità specificata più volte"},
    {"Duplicate option: ","Opzione duplicata: "},{"Unknown option: ","Opzione sconosciuta: "},
    {"Missing value for ","Valore mancante per "},{"Mode must be generate or calibrate","La modalità deve essere generate o calibrate"},
    {"Language must be en or it","La lingua deve essere en o it"}
  };
  for(const auto& entry:catalog) if(message.rfind(entry.first,0)==0)
    return entry.second+message.substr(entry.first.size());
  return "Dettaglio tecnico: "+message;
}
Inspection inspect(const std::filesystem::path& image) {
  const auto mat=readImage(image);
  return {mat.cols,mat.rows,mat.depth()==CV_16U?16:8};
}
EditableMergeLayout editableMergeLayout(
    const fs::path& configFile,const std::string& satellite,
    const std::array<fs::path,2>& alignedChannels) {
  require(!satellite.empty()&&satellite.size()<=32,"Specify a satellite/profile identifier");
  for(char c:satellite)
    require((c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_',"Invalid satellite identifier");
  const auto config=readConfig(configFile);
  std::array<cv::Mat,2> images{readImage(alignedChannels[0],false),
                               readImage(alignedChannels[1],false)};
  const auto key1=satellite+"VIS1",key2=satellite+"VIS2";
  EditableMergeLayout layout;
  layout.profileRoi=config[key1].isMember("ROI")&&config[key2].isMember("ROI");
  for(int i=0;i<2;++i) {
    const auto region=layout.profileRoi?roi(config[i?key2:key1],images[size_t(i)].size()):
                                        cv::Rect(0,0,images[size_t(i)].cols,images[size_t(i)].rows);
    layout.regions[size_t(i)]={region.x,region.y,region.width,region.height};
  }
  return layout;
}
void run(const RunOptions& options,const Log& log) {
  auto message=[&](const std::string& s){if(log)log(translate(s,options.language));};
  require(!options.satellite.empty() && options.satellite.size()<=32,
          "Specify a satellite/profile identifier");
  for(char c:options.satellite)
    require((c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_',"Invalid satellite identifier");
  require(!options.input.empty()&&!options.output.empty()&&!options.config.empty(),
          "Input, output and calibration profile are required");
  require(std::isfinite(options.calibrationScale) && options.calibrationScale>0 && options.calibrationScale<=1,
          "Calibration scale must be finite and in (0, 1]");
  require(options.calibrate || options.calibrationScale==1.0,"Calibration scale is only valid in calibrate mode");
  const auto input=fs::canonical(options.input);
  require(fs::is_directory(input), "Input must be a directory");
  const auto output=fs::absolute(options.output).lexically_normal();
  require(output!=input,"Input and output must differ");
  require(!fs::exists(fs::symlink_status(output)), "Output destination already exists");
  auto config=readConfig(options.config);
  // OpenCV ECC here consumes Mat, not UMat: selecting an OpenCL device does
  // not accelerate this estimator. Never label CPU ECC as GPU execution.
  const bool wantGpu=!options.cpu && !options.calibrate;
  const int gpu=wantGpu?openclAffineStatus():0;
  cv::ocl::setUseOpenCL(gpu==1);
  if(options.calibrate) message("Calibration backend: CPU (ECC)");
  else if(gpu==1) message("Backend: OpenCL");
  else {
    message("Backend: CPU");
    if(gpu==-1) message("OpenCL GPU affine diverges from CPU; using CPU backend");
  }
  const std::array<fs::path,2> dirs{input/"MSUGS_VIS1",input/"MSUGS_VIS2"};
  std::array<Channels,2> products;
  Staging stage(output);
  if(!options.products.empty()) {
    require(!options.calibrate,"Product selection is only valid in generate mode");
    require(options.products.size()<=32,"Too many selected VIS products");
    struct Selected {std::string folder;int id=0;bool aligned=false;Channels channels;};
    std::vector<Selected> selected;std::set<int> identifiers;
    for(const auto& folder:options.products) {
      require(!folder.empty()&&folder.size()<=64&&fs::path(folder).filename()==folder,
              "Invalid selected VIS product");
      const std::string prefix="MSUGS_VIS";bool aligned=false;std::string number;
      if(folder.rfind(prefix,0)==0) number=folder.substr(prefix.size());
      if(number.size()>8&&number.front()=='-'&&number.compare(number.size()-8,8,"_Aligned")==0) {
        aligned=true;number=number.substr(1,number.size()-9);
      }
      require(!number.empty()&&number.size()<=3&&number.front()!='0'&&
              std::all_of(number.begin(),number.end(),[](unsigned char c){return c>='0'&&c<='9';}),
              "Invalid selected VIS product");
      const int id=std::stoi(number);require(id>=1&&id<=999&&identifiers.insert(id).second,
              "Duplicate selected VIS product");
      const fs::path directory=input/folder;
      message("Loading "+folder);
      auto channels=load(directory,!aligned);
      if(!aligned) {
        const std::string key=options.satellite+"VIS"+std::to_string(id);
        require(config[key].isObject(),"No calibration for "+key);
        channels=align(channels,config[key]);
      }
      selected.push_back({folder,id,aligned,std::move(channels)});
    }
    for(const auto& product:selected) {
      const auto source=input/product.folder/"product.cbor";
      const auto destination=stage.path/("MSUGS_VIS-"+std::to_string(product.id)+"_Aligned");
      if(product.aligned) writeSelectedProduct(destination,product.channels,source);
      else writeProduct(destination,product.channels,source);
    }
    const auto findId=[&](int id){return std::find_if(selected.begin(),selected.end(),[&](const Selected& p){return p.id==id;});};
    const auto first=findId(1),second=findId(2);
    if(first!=selected.end()&&second!=selected.end()&&!first->aligned&&!second->aligned) {
      const auto key1=options.satellite+"VIS1",key2=options.satellite+"VIS2";
      if(config[key1].isMember("ROI")&&config[key2].isMember("ROI")) {
        const auto left=roi(config[key1],first->channels[0].size()),right=roi(config[key2],second->channels[0].size());
        require(first->channels[0].type()==second->channels[0].type(),"VIS products have different bit depths");
        require(left.width<=std::numeric_limits<int>::max()-right.width,"Merged dimensions overflow");
        Channels merged;
        for(int channel=0;channel<3;++channel) {
          merged[channel]=cv::Mat::zeros(std::max(left.height,right.height),left.width+right.width,first->channels[channel].type());
          first->channels[channel](left).copyTo(merged[channel](cv::Rect(0,0,left.width,left.height)));
          second->channels[channel](right).copyTo(merged[channel](cv::Rect(left.width,0,right.width,right.height)));
        }
        message("Writing merged aligned channels");
        writeProduct(stage.path/"MSUGS_VIS_Aligned",merged,input/first->folder/"product.cbor");
      }
    }
    message("Selected VIS products written for the editable compositor");
    stage.publish(output);message("Completed: "+output.string());return;
  }
  for(int vis=0;vis<2;++vis) {
    const std::string key=options.satellite+"VIS"+std::to_string(vis+1);
    message("Loading "+key);
    auto channels=load(dirs[vis]);
    if(options.calibrate) {
      message("Calibrating "+key);
      // Futures propagate worker exceptions to caller rather than terminating the process.
      if(options.calibrationScale<1.0)
        message("ECC working scale: "+std::to_string(options.calibrationScale)+"; transforms in native pixels");
      // Large real acquisitions otherwise run two memory-intensive ECC fits
      // concurrently. Serialization changes scheduling, not the estimator.
      if(channels[0].total()>16000000) {
        setTransform(config[key],"CH2",ecc(channels[0],channels[1],ch2offset,options.calibrationScale));
        setTransform(config[key],"CH3",ecc(channels[0],channels[2],ch3offset,options.calibrationScale));
      } else {
        auto second=std::async(std::launch::async,[&]{return ecc(channels[0],channels[1],ch2offset,options.calibrationScale);});
        auto third=std::async(std::launch::async,[&]{return ecc(channels[0],channels[2],ch3offset,options.calibrationScale);});
        setTransform(config[key],"CH2",second.get());
        setTransform(config[key],"CH3",third.get());
      }
    } else {
      require(config[key].isObject(),"No calibration for "+key);
      products[vis]=align(channels,config[key]);
      // Mat reference counting retains channel 1's buffer after channels leaves scope.
    }
  }
  if(options.calibrate) {
    std::ofstream stream(stage.path/"calibration.json",std::ios::binary);
    Json::StreamWriterBuilder writer; writer["indentation"]="  ";
    stream<<Json::writeString(writer,config)<<'\n';
    stream.close(); require(bool(stream),"Failed to save calibration profile");
    message("Calibrated profile; merge ROIs still require manual validation");
  } else {
    const auto key1=options.satellite+"VIS1",key2=options.satellite+"VIS2";
    if(config[key1].isMember("ROI")&&config[key2].isMember("ROI")) {
      const auto left=roi(config[key1],products[0][0].size());
      const auto right=roi(config[key2],products[1][0].size());
      require(products[0][0].type()==products[1][0].type(),"VIS products have different bit depths");
      require(left.width<=std::numeric_limits<int>::max()-right.width,"Merged dimensions overflow");
      Channels merged;
      for(int i=0;i<3;++i) {
        merged[i]=cv::Mat::zeros(std::max(left.height,right.height),left.width+right.width,products[0][i].type());
        products[0][i](left).copyTo(merged[i](cv::Rect(0,0,left.width,left.height)));
        products[1][i](right).copyTo(merged[i](cv::Rect(left.width,0,right.width,right.height)));
      }
      message("Writing merged aligned channels");
      writeProduct(stage.path/"MSUGS_VIS_Aligned",merged,dirs[0]/"product.cbor");
      if(options.editableParts) {
        writeProduct(stage.path/"MSUGS_VIS-1_Aligned",products[0],dirs[0]/"product.cbor");
        writeProduct(stage.path/"MSUGS_VIS-2_Aligned",products[1],dirs[1]/"product.cbor");
      }
    } else {
      message("No complete merge ROIs; writing separate aligned VIS products");
      writeProduct(stage.path/"MSUGS_VIS-1_Aligned",products[0],dirs[0]/"product.cbor");
      writeProduct(stage.path/"MSUGS_VIS-2_Aligned",products[1],dirs[1]/"product.cbor");
    }
    message("CBOR copied unchanged; SatDump geometry interoperability requires dataset validation");
  }
  stage.publish(output);
  message("Completed: "+output.string());
}
void duplicateProfile(const fs::path& source,const std::string& from,const std::string& to,
                      const fs::path& destination) {
  auto validId=[](const std::string& s){
    if(s.empty()||s.size()>32) return false;
    for(char c:s) if(!((c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_')) return false;
    return true;
  };
  require(validId(from)&&validId(to),"Invalid satellite identifier");
  require(from!=to,"Source and destination satellites must differ");
  const auto config=readConfig(source);
  Json::Value result=config;
  for(int vis=1;vis<=2;++vis) {
    const std::string f=from+"VIS"+std::to_string(vis), t=to+"VIS"+std::to_string(vis);
    require(config[f].isObject(),"No calibration for "+f);
    result[t]=config[f];
  }
  require(fs::is_directory(destination.parent_path()),"Output parent directory must exist");
  require(!fs::exists(fs::symlink_status(destination)),"Output destination already exists");
  // Write to a private temp file, then publish by exclusive atomic rename so a
  // concurrent creator of the destination can never be overwritten.
  std::random_device random;
  const fs::path temp=destination.parent_path()/
    (".orbyx-profile-"+std::to_string(random())+"-"+std::to_string(random()));
  {
    std::ofstream stream(temp,std::ios::binary|std::ios::trunc);
    Json::StreamWriterBuilder writer; writer["indentation"]="  ";
    stream<<Json::writeString(writer,result)<<'\n';
    stream.flush(); stream.close();
    if(!stream){ std::error_code ig; fs::remove(temp,ig); require(false,"Failed to save calibration profile"); }
  }
  bool published=false;
#ifdef __linux__
  published=::syscall(SYS_renameat2,AT_FDCWD,temp.c_str(),AT_FDCWD,destination.c_str(),1)==0;
#elif defined(__APPLE__)
  published=::renamex_np(temp.c_str(),destination.c_str(),RENAME_EXCL)==0;
#endif
  if(!published) {
    std::error_code ig; fs::remove(temp,ig);
#if defined(__linux__) || defined(__APPLE__)
    throw std::runtime_error("Cannot publish output exclusively: "+std::string(std::strerror(errno)));
#else
    throw std::runtime_error("Exclusive atomic publication is unsupported on this platform");
#endif
  }
}
namespace {
void composeProduct(const std::array<fs::path,3>& channels,const fs::path& requested,bool natural) {
  const auto destination=fs::absolute(requested).lexically_normal();
  std::array<cv::Mat,3> mats;
  for(int i=0;i<3;++i) mats[i]=readImage(channels[i],false);
  for(int i=1;i<3;++i)
    require(mats[i].size()==mats[0].size() && mats[i].type()==mats[0].type(),
            "All channels must share dimensions and depth for composition");
  require(fs::is_directory(destination.parent_path()),"Output parent directory must exist");
  require(!fs::exists(fs::symlink_status(destination)),"Output destination already exists");
  if(natural) require(destination.extension()==".png","Natural Color output must have a .png extension");
  cv::Mat rgb;
  if(!natural) {
    // Preserve the established generic RGB API's CH1->R mapping.
    std::vector<cv::Mat> bgr{mats[2],mats[1],mats[0]}; cv::merge(bgr,rgb);
  } else {
    // SatDump and the original aligner use RGB321 for MSU-GS. OpenCV is BGR.
    rgb.create(mats[0].size(),CV_MAKETYPE(mats[0].depth(),3));
    const double maximum=mats[0].depth()==CV_16U?65535.0:255.0;
    const int tileRows=std::max(1,std::min(128,1000000/mats[0].cols));
    for(int y=0;y<rgb.rows;y+=tileRows) {
      const auto region=cv::Rect(0,y,rgb.cols,std::min(tileRows,rgb.rows-y));
      std::vector<cv::Mat> bgr{mats[0](region),mats[1](region),mats[2](region)};
      cv::Mat native,original,hsv,corrected,blended;
      cv::merge(bgr,native);
      native.convertTo(original,CV_32F,1.0/maximum);
      cv::cvtColor(original,hsv,cv::COLOR_BGR2HSV);
      for(int row=0;row<hsv.rows;++row) {
        auto* pixels=hsv.ptr<cv::Vec3f>(row);
        for(int x=0;x<hsv.cols;++x) {
          float& hue=pixels[x][0];
          if(hue>=340.0f || hue<=40.0f) hue+=60.0f;
          else if(hue<=100.0f) hue-=6.0f;
          if(hue>=360.0f) hue-=360.0f;
        }
      }
      cv::cvtColor(hsv,corrected,cv::COLOR_HSV2BGR);
      cv::addWeighted(original,0.6,corrected,0.4,0,blended);
      blended.convertTo(rgb(region),rgb.type(),maximum);
    }
  }
  writeCompositePng(rgb,destination);
}
}
cv::Mat readCompositePng(const fs::path& path) { return readImage(path,false,true); }
void writeCompositePng(const cv::Mat& rgb,const fs::path& requested) {
  const auto destination=fs::absolute(requested).lexically_normal();
  require(destination.extension()==".png","Composite output must have a .png extension");
  require(!rgb.empty()&&rgb.channels()==3&&(rgb.depth()==CV_8U||rgb.depth()==CV_16U),"Invalid RGB composite");
  // A private directory protects the encoder target from symlink substitution;
  // RAII also removes temporary output if imwrite throws or publication loses a race.
  Staging stage(destination);
  const fs::path temp=stage.path/"composite.png";
  require(cv::imwrite(temp.string(),rgb),"Failed to encode composed RGB product");
  bool published=false;
#ifdef __linux__
  published=::syscall(SYS_renameat2,AT_FDCWD,temp.c_str(),AT_FDCWD,destination.c_str(),1)==0;
#elif defined(__APPLE__)
  published=::renamex_np(temp.c_str(),destination.c_str(),RENAME_EXCL)==0;
#endif
  if(!published) {
    std::error_code ig; fs::remove(temp,ig);
#if defined(__linux__) || defined(__APPLE__)
    throw std::runtime_error("Cannot publish output exclusively: "+std::string(std::strerror(errno)));
#else
    throw std::runtime_error("Exclusive atomic publication is unsupported on this platform");
#endif
  }
}

void composeRgb(const std::array<fs::path,3>& channels,const fs::path& destination) {
  composeProduct(channels,destination,false);
}
void composeSingleChannel(const std::array<fs::path,3>& channels,const fs::path& destination,int index) {
  require(index>=0&&index<3,"Single-channel index must be 0, 1 or 2");
  const cv::Mat channel=readImage(channels[size_t(index)],false);
  std::vector<cv::Mat> bgr{channel,channel,channel};cv::Mat gray;cv::merge(bgr,gray);
  writeCompositePng(gray,destination);
}
void composeNaturalColor(const std::array<fs::path,3>& channels,const fs::path& destination) {
  composeProduct(channels,destination,true);
}
}
