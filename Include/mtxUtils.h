#ifndef MTXUTILS_H
#define MTXUTILS_H
#include "NRIFramework.h"
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
namespace utils {
nri::AccessBits bufferUsageToAccess(nri::BufferUsageBits usage);
}// namespace utils
}// namespace MTX

#endif//MTXUTILS_H