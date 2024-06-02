#ifndef MTXUTILS_H
#define MTXUTILS_H
#include "NRIFramework.h"
#include <glm/glm.hpp>
namespace MTX {
const uint32_t MTX_MAX_FRAME_COUNT = BUFFERED_FRAME_MAX_NUM;
struct MtxRayTracingPushConstant {
  int temp;
};
struct RtInstanceInfo {
  uint32_t indexOffset = 0;
  uint32_t vertexOffset = 0;
  uint32_t vertexCount = 0;
  uint32_t indexCount = 0;
  uint32_t meshIdx = 0;
};

struct CameraUniform {
  float4x4 clipToView;
  float4x4 viewToWorld;
  //vec3 camera pos,and float fov
  float4 camPosFov;
};
namespace utils {
nri::AccessBits bufferUsageToAccess(nri::BufferUsageBits usage);
}// namespace utils
}// namespace MTX

#endif//MTXUTILS_H