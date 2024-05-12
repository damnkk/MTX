#include "interface.h"
#include "log.h"
#include "resourcePool.h"

namespace MTX {

MTXInterface::MTXInterface() {
  m_textureAllocator = std::make_shared<TextureAllocator>(this);
  m_bufferAllocator = std::make_shared<BufferAllocator>(this);
  m_acceStructureAllocator = std::make_shared<AcceStructureAllocator>(this);
}
void MTXInterface::destroy() {
  DestroyCommandAllocator(*_instantCommandAllocator);
  m_textureAllocator->destroy();
  m_bufferAllocator->destroy();
  m_acceStructureAllocator->destroy();
}

std::shared_ptr<MtxTexture> MTXInterface::allocateTexture(const MtxTextureAllocInfo& allocInfo) {
  return m_textureAllocator->allocateTexture(allocInfo);
}

std::shared_ptr<MtxBuffer> MTXInterface::allocateBuffer(const MtxBufferAllocInfo& allocInfo) {
  return m_bufferAllocator->allocateBuffer(allocInfo);
}

std::shared_ptr<MtxAcceStructure>
MTXInterface::allocateAccStructure(const nri::AccelerationStructureDesc& allocInfo) {
  return m_acceStructureAllocator->allocateAcceStructure(allocInfo);
}

std::shared_ptr<MtxPipeline>
MTXInterface::allocatePipeline(const MtxPipelineAllocateInfo& allocInfo) {
  return m_pipelineAllocator->allocatePipeline(allocInfo);
}

nri::CommandBuffer* MTXInterface::getInstantCommandBuffer() {
  std::lock_guard<std::mutex> lock(_mutex);
  nri::CommandBuffer*         cmdBuf = nullptr;
  auto                        res = CreateCommandBuffer(*_instantCommandAllocator, cmdBuf);
  MTX_ASSERT(res == nri::Result::SUCCESS);
  BeginCommandBuffer(*cmdBuf, nullptr);
  return cmdBuf;
}

void MTXInterface::flushInstantCommandBuffer(nri::CommandBuffer* cmdBuf) {
  EndCommandBuffer(*cmdBuf);
  nri::QueueSubmitDesc desc{.commandBuffers = &cmdBuf, .commandBufferNum = 1};
  QueueSubmit(*_graphicQueue, desc);
  WaitForIdle(*_graphicQueue);
  DestroyCommandBuffer(*cmdBuf);
}

}// namespace MTX