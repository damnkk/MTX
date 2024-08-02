#ifndef INTERFACE_H
#define INTERFACE_H
// clang-format off
// #include "NRIFramework.h"
#include "NRI.h"
#include "Extensions/NRIDeviceCreation.h"
#include "Extensions/NRIHelper.h"
#include "Extensions/NRIStreamer.h"
#include "Extensions/NRISwapChain.h"

#include "Extensions/NRIRayTracing.h"
#include <mutex>
//#include "resourcePool.h"
// clang-format on

namespace MTX {
struct TextureAllocator;
struct MtxTexture;
struct MtxTextureAllocInfo;
struct BufferAllocator;
struct MtxBuffer;
struct MtxBufferAllocInfo;
struct AcceStructureAllocator;
struct MtxAcceStructure;
struct PipelineAllocator;
struct MtxPipelineAllocateInfo;
struct MtxPipeline;
struct MTXInterface : public nri::CoreInterface,
                      public nri::StreamerInterface,
                      public nri::SwapChainInterface,
                      public nri::HelperInterface,
                      public nri::RayTracingInterface {
  MTXInterface();
  void                        destroy();
  std::shared_ptr<MtxTexture> allocateTexture(const MtxTextureAllocInfo& allocInfo);
  std::shared_ptr<MtxBuffer>  allocateBuffer(const MtxBufferAllocInfo& allocInfo);
  std::shared_ptr<MtxAcceStructure>
  allocateAccStructure(const nri::AccelerationStructureDesc& allocInfo);
  std::shared_ptr<MtxPipeline> allocatePipeline(const MtxPipelineAllocateInfo& allocinfo);

  nri::Device&           getDevice() { return *(_device); }
  nri::CommandQueue&     getGraphicQueue() { return *(_graphicQueue); }
  nri::CommandQueue&     getComputeQueue() { return *(_computeQueue); }
  nri::CommandQueue&     getTransferQueue() { return *(_transferQueue); }
  nri::CommandBuffer*    getInstantCommandBuffer();
  void                   flushInstantCommandBuffer(nri::CommandBuffer* cmdBuf);
  nri::CommandBuffer*    getCommandBuffer(uint32_t frameIndex, bool begin = true);
  nri::Device*           _device = nullptr;
  nri::CommandQueue*     _graphicQueue = nullptr;
  nri::CommandQueue*     _computeQueue = nullptr;
  nri::CommandQueue*     _transferQueue = nullptr;
  nri::CommandAllocator* _instantCommandAllocator = nullptr;
  std::mutex             _mutex;

  std::shared_ptr<TextureAllocator>       m_textureAllocator;
  std::shared_ptr<BufferAllocator>        m_bufferAllocator;
  std::shared_ptr<AcceStructureAllocator> m_acceStructureAllocator;
  std::shared_ptr<PipelineAllocator>      m_pipelineAllocator;
};

}// namespace MTX

#endif//INTERFACE_H