#ifndef RENDERER_H
#define RENDERER_H
#include "interface.h"
#include "mtxCamera.h"
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
  void initCamera();
  void createRayTracingTex(nri::Format fmt);
  void createSwapChain(nri::Format& fmt);
  void createRayTracingPipeline();
  void createDescriptorSets();
  void updateDescriptorSets();
  void createTLAS();
  void createBLAS();
  void createSBT();

  void updateCamera(float deltaTime);
  void frameResize();

 private:
  MTXInterface                                   m_interface = {};
  nri::SwapChain*                                m_swapChain = nullptr;
  nri::Streamer*                                 m_streamer = nullptr;
  nri::QueryPool*                                m_queryPool = nullptr;
  nri::DescriptorPool*                           m_descriptorPool = nullptr;
  nri::Descriptor*                               m_sampler = nullptr;
  std::shared_ptr<MtxPipeline>                   m_rayTracingPipeline = nullptr;
  std::vector<FrameResource>                     m_frameResource;
  std::shared_ptr<SceneLoader>                   m_sceneLoader = nullptr;
  std::shared_ptr<MtxAcceStructure>              m_tlas;
  std::vector<std::shared_ptr<MtxAcceStructure>> m_blas;
  std::shared_ptr<MtxBuffer>                     m_shaderBindingTable;
  std::vector<nri::DescriptorSet*>               m_descriptorSets;
  std::shared_ptr<MtxTexture>                    m_rayTracingTexture;
  //the main camera is in sampleBase,cameras down here is prepare for multi-camera rendering
  /*
  warning!!!: the currently used function GetCameraDescFromInputDevices is m_MainCamera's exclusion,
  for multi-camera systems, you can not udpate camera state by using this function.
  */
  std::vector<MtxCamera> m_cameras;
  struct MtxRayTracingPushConstant {
    uint32_t accumFrameCount = 0;
    uint32_t maxSampleCount = INT_MAX;
    uint32_t maxBounce = 8;
  } m_constant;
  uint32_t m_currentFrame = 0;
  uint32_t m_frameIndex = 0;
  uint64_t m_shaderGroupIdentifierSize = 0;
  uint64_t m_missShaderOffset = 0;
  uint64_t m_hitShaderGroupOffset = 0;
  uint32_t m_threadNum = 12;
  uint32_t m_maxBounce = 3;
};
}// namespace MTX

#endif//RENDERER_H