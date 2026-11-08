// ORBYX — GPL-3.0
#include "orbyx/engine.hpp"
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#ifndef ORBYX_VERSION
#define ORBYX_VERSION "0.1.0"
#endif
namespace {
void help(const std::string& language) {
  if(language=="it") {
    std::cout << "ORBYX " ORBYX_VERSION " — allineamento canali RDAS\n"
      "Uso: orbyx [generate|calibrate] -s SAT -d INPUT --output NUOVA_CARTELLA [opzioni]\n"
      "  -s, --sat          Satellite/profilo (L2, L3, L4, L5, M1, M2)\n"
      "  -d, --directory    Cartella RDAS decodificata in tempo reale da SatDump\n"
      "  -m, --mode         generate (predefinita) oppure calibrate\n"
      "      --config       JSON di calibrazione (predefinito: config.json)\n"
      "      --output       Nuova cartella di uscita; non deve esistere\n"
      "      --cpu          Disattiva OpenCL e utilizza la CPU\n"
      "      --lang         Lingua en oppure it\n"
      "  -h, --help         Mostra questa guida\n"
      "      --version      Mostra la versione\n"
      "calibrate salva OUTPUT/calibration.json. Gli input originali restano invariati.\n"
      "Natural Color non disponibile: disabilitato nell'implementazione originale.\n";
    return;
  }
  std::cout << "ORBYX " ORBYX_VERSION " — RDAS channel alignment\n"
    "Usage: orbyx [generate|calibrate] -s SAT -d INPUT --output NEW_DIRECTORY [options]\n"
    "  -s, --sat          Satellite/profile (L2, L3, L4, L5, M1, M2)\n"
    "  -d, --directory    SatDump live-decoded RDAS input directory\n"
    "  -m, --mode         generate (default) or calibrate\n"
    "      --config       Calibration JSON (default: config.json)\n"
    "      --output       New output directory; must not already exist\n"
    "      --cpu          Disable OpenCL (CPU fallback)\n"
    "      --lang         Language en or it\n"
    "  -h, --help         Show help\n"
    "      --version      Print version\n"
    "Calibrate writes OUTPUT/calibration.json. Original inputs remain unchanged.\n"
    "No Natural Color export: it was disabled in the upstream implementation.\n";
}
}
int main(int argc,char** argv) {
  std::string language="en";
  // Resolve locale before help/errors, independently of option order.
  for(int i=1;i+1<argc;++i) if(std::string(argv[i])=="--lang") language=argv[i+1];
  try {
    if(language!="en"&&language!="it") throw std::runtime_error("Language must be en or it");
    orbyx::RunOptions options;
    options.config="config.json";
    options.language=language;
    std::string mode="generate";
    bool modeSet=false;
    std::map<std::string,bool> seen;
    const std::map<std::string,std::string> aliases{{"-s","--sat"},{"-d","--directory"},{"-m","--mode"}};
    if(argc==1) {help(language);return 0;}
    for(int i=1;i<argc;++i) {
      std::string flag=argv[i];
      if(flag=="--help"||flag=="-h") {help(language);return 0;}
      if(flag=="--version") {std::cout<<"ORBYX " ORBYX_VERSION "\n";return 0;}
      if(flag=="generate"||flag=="calibrate") {
        if(modeSet) throw std::runtime_error("Mode specified more than once");
        mode=flag;modeSet=true;continue;
      }
      if(aliases.count(flag)) flag=aliases.at(flag);
      if(seen[flag]) throw std::runtime_error("Duplicate option: "+flag);
      seen[flag]=true;
      if(flag=="--cpu") {options.cpu=true;continue;}
      if(flag!="--sat"&&flag!="--directory"&&flag!="--mode"&&flag!="--config"&&flag!="--output"&&flag!="--lang")
        throw std::runtime_error("Unknown option: "+flag);
      if(i+1>=argc) throw std::runtime_error("Missing value for "+flag);
      const std::string value=argv[++i];
      if(value.empty()||value.rfind("--",0)==0) throw std::runtime_error("Missing value for "+flag);
      if(flag=="--lang") continue;
      if(flag=="--sat") options.satellite=value;
      else if(flag=="--directory") options.input=value;
      else if(flag=="--output") options.output=value;
      else if(flag=="--config") options.config=value;
      else {
        if(modeSet) throw std::runtime_error("Mode specified more than once");
        mode=value;modeSet=true;
      }
    }
    if(mode!="generate"&&mode!="calibrate") throw std::runtime_error("Mode must be generate or calibrate");
    options.calibrate=mode=="calibrate";
    orbyx::run(options,[](const std::string& message){std::cout<<message<<'\n';});
    return 0;
  } catch(const std::exception& error) {
    std::cerr<<"ORBYX: "<<orbyx::translate(error.what(),language)<<'\n';return 2;
  }
}
