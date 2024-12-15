#ifndef MTXUTILS_H
#define MTXUTILS_H
#include "NRIFramework.h"
#include<Interface.h>
namespace MTX {
const uint32_t MTX_MAX_FRAME_COUNT = BUFFERED_FRAME_MAX_NUM;
struct ShaderLoader{
  ShaderLoader(MTXInterface* interface) : _interface(interface){}
  ShaderLoader& addShader(std::string path,const char* introName, ::utils::ShaderCodeStorage& shaderCodeStorage){

    shaderDescs.emplace_back(::utils::LoadShader(_interface->GetDeviceDesc(_interface->getDevice()).graphicsAPI,path,shaderCodeStorage,introName));
    if((int(shaderDescs.back().stage)|int(nri::StageBits::RAY_TRACING_SHADERS))!=0){
      shaderTypeNum[int(shaderDescs.back().stage)>>12]+=1;
    }
    return *this;
  }
  std::vector<nri::ShaderDesc>& getShaderDesc() { return shaderDescs; }
  std::array<int,6> getShaderTypeNum() { return shaderTypeNum; }
  std::vector<nri::ShaderDesc> shaderDescs;
  std::array<int,6> shaderTypeNum={};
  MTXInterface* _interface;
};
struct RtInstanceInfo {
  uint32_t indexOffset = 0;
  uint32_t vertexOffset = 0;
  uint32_t vertexCount = 0;
  uint32_t indexCount = 0;
  uint32_t meshIdx = 0;
};

struct CameraUniform {
  float4x4 ViewToClipPrev;
  float4x4 ClipToViewPrev;
  float4x4 WorldToViewPrev;
  float4x4 ViewToWorldPrev;
  float4x4 WorldToClipPrev;
  float4x4 ClipToWorldPrev;
  float4x4 ViewToClip;
  float4x4 ClipToView;
  float4x4 WorldToView;
  float4x4 ViewToWorld;
  float4x4 WorldToClip;
  float4x4 ClipToWorld;

  //vec3 camera pos,and float fov
  float4 camPosFov;

  //light data
  float4 sunBasisX;
  float4 sunBasisY;
  float4 sunDirection;
  float  tanSunAngularRadius;
};
namespace utils {
nri::AccessBits bufferUsageToAccess(nri::BufferUsageBits usage);
}// namespace utils
}// namespace MTX

#endif//MTXUTILS_H