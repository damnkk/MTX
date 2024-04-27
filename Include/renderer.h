#include "interface.h"
#include "resourcePool.h"
#include <NRIFramework.h>

namespace MTX {

class MTXRtRenderer : public SampleBase {
 public:
  MTXRtRenderer() {}
  ~MTXRtRenderer();

  virtual bool Initialize(nri::GraphicsAPI graphicsAPI) override;
  virtual void PrepareFrame(uint32_t frameIndex) override;
  virtual void RenderFrame(uint32_t frameIndex) override;

 protected:
  void createSwapChain(nri::Format& fmt);
  void createCommandBuffer();
  void createRayTracingPipeline();
  void createTLAS();
  void createBLAS();
  void createSBT();

 private:
  MTXInterface               m_interface = {};
  nri::Streamer*             m_streamer = nullptr;
  nri::SwapChain*            m_swapChain = nullptr;
  std::vector<FrameResource> m_frameResource;
  nri::DescriptorPool*       m_descriptorPool = nullptr;
  nri::QueryPool*            m_queryPool = nullptr;
  TextureAllocator           m_textureAllocator;
  BufferAllocator            m_bufferAllocator;
  uint32_t                   m_threadNum = 16;
};
}// namespace MTX