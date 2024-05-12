#ifndef RENDERER_H
#define RENDERER_H
#include "interface.h"
#include "resourcePool.h"
#include <NRIFramework.h>

namespace MTX {
struct SceneLoader;
class MTXRenderer : public SampleBase {
 public:
  MTXRenderer() {}
  ~MTXRenderer();

  virtual bool                 Initialize(nri::GraphicsAPI graphicsAPI) override;
  virtual void                 PrepareFrame(uint32_t frameIndex) override;
  virtual void                 RenderFrame(uint32_t frameIndex) override;
  std::shared_ptr<SceneLoader> getSceneLoader() { return m_sceneLoader; }

 protected:
  void createSwapChain(nri::Format& fmt);
  void createCommandBuffer();
  void createRayTracingPipeline();
  void createDescriptorSets();
  void createTLAS();
  void createBLAS();
  void createSBT();

 private:
  MTXInterface                                   m_interface = {};
  nri::Streamer*                                 m_streamer = nullptr;
  nri::SwapChain*                                m_swapChain = nullptr;
  std::vector<FrameResource>                     m_frameResource;
  nri::DescriptorPool*                           m_descriptorPool = nullptr;
  nri::QueryPool*                                m_queryPool = nullptr;
  uint32_t                                       m_threadNum = 16;
  std::shared_ptr<SceneLoader>                   m_sceneLoader = nullptr;
  std::shared_ptr<MtxAcceStructure>              m_tlas;
  std::vector<std::shared_ptr<MtxAcceStructure>> m_blas;
  nri::Descriptor*                               m_tlasDescriptor;
  nri::DescriptorSet*                            m_tlasDescriptorSet;
  std::vector<nri::DescriptorSet*>               m_descriptorSets;
};
}// namespace MTX

#endif//RENDERER_H