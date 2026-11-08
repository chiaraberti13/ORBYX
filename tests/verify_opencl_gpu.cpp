// GPL-3.0. Hardware capability gate: never reports a CPU fallback as a GPU pass.
#include <opencv2/core.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/imgproc.hpp>
#include <json/json.h>
#include <iostream>
#include <stdexcept>

int main() {
 Json::Value report;
 report["whole_engine_gpu_verified"]=false;
 report["ecc_gpu_verified"]=false; // ORBYX ECC uses Mat and is a CPU estimator.
 auto print=[&](int code) {Json::StreamWriterBuilder w;w["indentation"]="  ";std::cout<<Json::writeString(w,report)<<'\n';return code;};
 try {
  report["have_opencl"]=cv::ocl::haveOpenCL();
  // OpenCV 4.13 throws from getPlatfomsInfo when clGetPlatformIDs is not
  // available. Missing runtime is an unavailable capability, not a kernel fault.
  if(!cv::ocl::haveOpenCL()) {
   report["platform_count"]=0;report["device_count"]=0;report["gpu_device_count"]=0;
   report["status"]="skipped";report["reason"]="OpenCL runtime is unavailable; no GPU execution can be tested";
   return print(77);
  }
  std::vector<cv::ocl::PlatformInfo> platforms;
  cv::ocl::getPlatfomsInfo(platforms);
  report["platform_count"]=static_cast<unsigned>(platforms.size());
  unsigned devices=0,gpus=0;
  for(const auto& platform:platforms) {
   for(int i=0;i<platform.deviceNumber();++i) {
    cv::ocl::Device device;platform.getDevice(device,i);++devices;
    if(device.type()&cv::ocl::Device::TYPE_GPU)++gpus;
   }
  }
  report["device_count"]=devices;report["gpu_device_count"]=gpus;
  if(gpus==0) {
   report["status"]="skipped";report["reason"]="No real OpenCL GPU is available; CPU fallback is not a GPU test";
   return print(77);
  }
  cv::ocl::setUseOpenCL(true);
  const auto& device=cv::ocl::Device::getDefault();
  report["selected_device"]=device.name();report["vendor"]=device.vendorName();report["driver"]=device.driverVersion();
  if(!cv::ocl::useOpenCL() || !(device.type()&cv::ocl::Device::TYPE_GPU) || !device.available()) {
   report["status"]="skipped";report["reason"]="Selected backend is not an available GPU; set OPENCV_OPENCL_DEVICE=:GPU:";
   return print(77);
  }
  const cv::ocl::ProgramSource source("orbyx","gpu_proof",
   "__kernel void proof(__global const float* input, __global float* output) { int i=get_global_id(0); output[i]=input[i]*3.0f+2.0f; }", "");
  cv::String buildError;
  cv::ocl::Kernel kernel("proof",source,"",&buildError);
  if(kernel.empty())throw std::runtime_error("OpenCL GPU kernel build failed: "+buildError);
  cv::Mat input(1,4096,CV_32F);
  for(int i=0;i<input.cols;++i)input.at<float>(0,i)=static_cast<float>(i);
  cv::UMat sourceGpu,targetGpu;input.copyTo(sourceGpu);targetGpu.create(input.size(),input.type());
  kernel.args(cv::ocl::KernelArg::PtrReadOnly(sourceGpu),cv::ocl::KernelArg::PtrWriteOnly(targetGpu));
  size_t global[]={4096};
  if(!kernel.run(1,global,nullptr,true))throw std::runtime_error("OpenCL GPU kernel execution failed");
  cv::Mat actual;targetGpu.copyTo(actual);
  for(int i=0;i<actual.cols;++i)
   if(actual.at<float>(0,i)!=i*3.0f+2.0f)throw std::runtime_error("OpenCL GPU kernel produced an incorrect value");
  report["explicit_gpu_kernel_verified"]=true;
  for(const int depth:{CV_8U,CV_16U}) {
   cv::Mat frame(257,383,depth);cv::RNG random(981);random.fill(frame,cv::RNG::UNIFORM,0,depth==CV_8U?255:65535);
   cv::GaussianBlur(frame,frame,cv::Size(9,9),2);
   const cv::Mat transform=(cv::Mat_<float>(2,3)<<1.001,0.002,2.125,-0.001,0.999,1.5);
   cv::Mat cpu,gpu;
   cv::ocl::setUseOpenCL(false);
   cv::warpAffine(frame,cpu,transform,frame.size(),cv::INTER_LINEAR|cv::WARP_INVERSE_MAP);
   cv::ocl::setUseOpenCL(true);
   cv::UMat nativeFrame,nativeOutput;frame.copyTo(nativeFrame);
   cv::warpAffine(nativeFrame,nativeOutput,transform,frame.size(),cv::INTER_LINEAR|cv::WARP_INVERSE_MAP);
   nativeOutput.copyTo(gpu);
   const double error=cv::norm(cpu,gpu,cv::NORM_INF);
   report[depth==CV_8U?"affine_8bit_max_error":"affine_16bit_max_error"]=error;
   if(error>1)throw std::runtime_error("Requested OpenCL affine differs from CPU by more than one level");
  }
  report["status"]="gpu_kernel_verified";
  report["scope"]="Explicit OpenCL kernel executed on a GPU; requested OpenCV UMat affine compared. OpenCV may internally fall back for individual operations; full engine dispatch/profiling and performance are separate gates.";
  return print(0);
 } catch(const std::exception& error) {
  report["status"]="failed";report["error"]=error.what();return print(1);
 }
}
