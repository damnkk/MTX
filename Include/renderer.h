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
  void createRayTracingPipeline();
  void createDescriptorSets();
  void createTLAS();
  void createBLAS();
  void createSBT();

 private:
  MTXInterface                                   m_interface = {};
  nri::Streamer*                                 m_streamer = nullptr;
  std::shared_ptr<MtxPipeline>                   m_rayTracingPipeline = nullptr;
  nri::SwapChain*                                m_swapChain = nullptr;
  std::vector<FrameResource>                     m_frameResource;
  nri::DescriptorPool*                           m_descriptorPool = nullptr;
  nri::QueryPool*                                m_queryPool = nullptr;
  uint32_t                                       m_threadNum = 16;
  std::shared_ptr<SceneLoader>                   m_sceneLoader = nullptr;
  std::shared_ptr<MtxAcceStructure>              m_tlas;
  std::vector<std::shared_ptr<MtxAcceStructure>> m_blas;
  std::shared_ptr<MtxBuffer>                     m_shaderBindingTable;
  std::vector<nri::DescriptorSet*>               m_descriptorSets;
  uint64_t                                       m_shaderGroupIdentifierSize = 0;
  uint64_t                                       m_missShaderOffset = 0;
  uint64_t                                       m_hitShaderGroupOffset = 0;
};
}// namespace MTX

#endif//RENDERER_H