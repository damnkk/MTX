#ifndef INTERFACE_H
#define INTERFACE_H
// clang-format off
#include "NRIFramework.h"
#include "Extensions/NRIRayTracing.h"
#include <mutex>
// clang-format on

namespace MTX {
struct MTXInterface : public nri::CoreInterface,
                      public nri::StreamerInterface,
                      public nri::SwapChainInterface,
                      public nri::HelperInterface,
                      public nri::RayTracingInterface,
                      public nri::MemoryAllocatorInterface {
  nri::Device&           getDevice() { return *(_device); }
  nri::CommandQueue&     getGraphicQueue() { return *(_graphicQueue); }
  nri::CommandQueue&     getComputeQueue() { return *(_computeQueue); }
  nri::CommandQueue&     getTransferQueue() { return *(_transferQueue); }
  void                   destroy() { DestroyCommandAllocator(*_instantCommandAllocator); }
  nri::CommandBuffer*    getInstantCommandBuffer();
  nri::CommandBuffer*    getCommandBuffer(uint32_t frameIndex, bool begin = true);
  nri::Device*           _device = nullptr;
  nri::CommandQueue*     _graphicQueue = nullptr;
  nri::CommandQueue*     _computeQueue = nullptr;
  nri::CommandQueue*     _transferQueue = nullptr;
  nri::CommandAllocator* _instantCommandAllocator = nullptr;
  std::mutex             _mutex;
};

}// namespace MTX

#endif//INTERFACE_H