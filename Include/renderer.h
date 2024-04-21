#include "interface.h"
#include <NRIFramework.h>

namespace MTX {

class MTXRtRenderer : public SampleBase {
 public:
  MTXRtRenderer() {}
  ~MTXRtRenderer();

  virtual bool Initialize(nri::GraphicsAPI graphicsAPI) override;
  virtual void PrepareFrame(uint32_t frameIndex) override;
  virtual void RenderFrame(uint32_t frameIndex) override;

 private:
  MTXInterface             m_interface = {};
  nri::Device*             m_device = nullptr;
  nri::Streamer*           m_streamer = nullptr;
  nri::SwapChain*          m_swapchain = nullptr;
  nri::CommandQueue*       m_graphicsQueue = nullptr;
  nri::CommandQueue*       m_computeQueue = nullptr;
  nri::CommandQueue*       m_transferQueue = nullptr;
  std::vector<nri::Fence*> m_frameFence;
  nri::DescriptorPool*     m_descriptorPool = nullptr;
  nri::QueryPool*          m_queryPool = nullptr;
};
}// namespace MTX