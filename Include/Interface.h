#ifndef INTERFACE_H
#define INTERFACE_H
#include "NRIFramework.h"
#include <mutex>
namespace MTX {
struct MTXInterface : public nri::CoreInterface,
                      public nri::StreamerInterface,
                      public nri::SwapChainInterface,
                      public nri::HelperInterface {
  nri::Device&           getDevice() { return *(_device); }
  nri::CommandQueue&     getGraphicQueue() { return *(_graphicQueue); }
  nri::CommandQueue&     getComputeQueue() { return *(_computeQueue); }
  nri::CommandQueue&     getTransferQueue() { return *(_transferQueue); }
  void                   destroy() { DestroyCommandAllocator(*_instanceCommandAllocator); }
  nri::CommandBuffer*    getInstanceCommandBuffer();
  nri::Device*           _device = nullptr;
  nri::CommandQueue*     _graphicQueue = nullptr;
  nri::CommandQueue*     _computeQueue = nullptr;
  nri::CommandQueue*     _transferQueue = nullptr;
  nri::CommandAllocator* _instanceCommandAllocator = nullptr;
  std::mutex             _mutex;
};

}// namespace MTX

#endif//INTERFACE_H