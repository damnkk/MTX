#include "renderer.h"
#include "log.h"
#include "mtxUtils.h"

namespace MTX {

MTXRtRenderer::~MTXRtRenderer() {
  m_interface.WaitForIdle(m_interface.getGraphicQueue());
  m_interface.WaitForIdle(m_interface.getComputeQueue());
  m_interface.WaitForIdle(m_interface.getTransferQueue());
}

bool MTXRtRenderer::Initialize(nri::GraphicsAPI graphicsAPI) {

  nri::AdapterDesc bestAdaptDesc = {};
  uint32_t         adapterDescsNum = 1;
  auto             res = nri::nriEnumerateAdapters(&bestAdaptDesc, adapterDescsNum);
  MTX_CHECK(res);
  nri::DeviceCreationDesc deviceCreationDesc = {};
  deviceCreationDesc.adapterDesc = &bestAdaptDesc;
  deviceCreationDesc.memoryAllocatorInterface = m_MemoryAllocatorInterface;
  deviceCreationDesc.graphicsAPI = graphicsAPI;
  deviceCreationDesc.spirvBindingOffsets = SPIRV_BINDING_OFFSETS;
  deviceCreationDesc.enableAPIValidation = m_DebugAPI;
  deviceCreationDesc.enableNRIValidation = m_DebugNRI;
  res = nri::nriCreateDevice(deviceCreationDesc, m_interface._device);
  MTX_CHECK(res)
  MTX_CHECK(nri::nriGetInterface(m_interface.getDevice(), NRI_INTERFACE(nri::CoreInterface),
                                 (nri::CoreInterface*) (&m_interface)));
  MTX_CHECK(nri::nriGetInterface(m_interface.getDevice(), NRI_INTERFACE(nri::HelperInterface),
                                 (nri::HelperInterface*) (&m_interface)));
  MTX_CHECK(nri::nriGetInterface(m_interface.getDevice(), NRI_INTERFACE(nri::RayTracingInterface),
                                 (nri::RayTracingInterface*) (&m_interface)));
  MTX_CHECK(nri::nriGetInterface(m_interface.getDevice(), NRI_INTERFACE(nri::StreamerInterface),
                                 (nri::StreamerInterface*) (&m_interface)));
  MTX_CHECK(nri::nriGetInterface(m_interface.getDevice(), NRI_INTERFACE(nri::SwapChainInterface),
                                 (nri::SwapChainInterface*) (&m_interface)));
  MTX_CHECK(nri::nriGetInterface(m_interface.getDevice(),
                                 NRI_INTERFACE(nri::MemoryAllocatorInterface),
                                 (nri::MemoryAllocatorInterface*) (&m_interface)));
  m_textureAllocator.setInterface(&m_interface);
  m_bufferAllocator.setInterface(&m_interface);
  MTX_CHECK(m_interface.GetCommandQueue(m_interface.getDevice(), nri::CommandQueueType::GRAPHICS,
                                        m_interface._graphicQueue));
  MTX_CHECK(m_interface.GetCommandQueue(m_interface.getDevice(), nri::CommandQueueType::COMPUTE,
                                        m_interface._computeQueue));
  MTX_CHECK(m_interface.GetCommandQueue(m_interface.getDevice(), nri::CommandQueueType::COPY,
                                        m_interface._transferQueue));
  m_frameResource.resize(MTX_MAX_FRAME_COUNT);
  for (auto& i : m_frameResource) {
    MTX_CHECK(m_interface.CreateFence(m_interface.getDevice(), 0, i._fence));
  }
  createCommandBuffer();
  nri::Format swpFormat = nri::Format::UNKNOWN;
  createSwapChain(swpFormat);
  // createRayTracingPipeline();
  // createSBT();

  MTX_INFO("----MTXRtRenderer initialized success-----")

  return InitUI(m_interface, m_interface, m_interface.getDevice(), swpFormat);
}

void MTXRtRenderer::createCommandBuffer() {
  for (auto& i : m_frameResource) {
    MTX_CHECK(m_interface.CreateCommandAllocator(m_interface.getGraphicQueue(), i._commandPool));
    for (int idx = 0; idx < m_threadNum; ++idx) {
      MTX_CHECK(m_interface.CreateCommandBuffer(*(i._commandPool), i._commandBuffers[idx]));
    }
  }
}

void MTXRtRenderer::createSwapChain(nri::Format& format) {
  nri::SwapChainDesc swapDesc{};
  swapDesc.window = GetWindow();
  swapDesc.width = (nri::Dim_t) GetWindowResolution().x;
  swapDesc.height = (nri::Dim_t) GetWindowResolution().y;
  swapDesc.format = nri::SwapChainFormat::BT709_G22_8BIT;
  swapDesc.commandQueue = m_interface._graphicQueue;
  swapDesc.textureNum = MTX_MAX_FRAME_COUNT;
  swapDesc.verticalSyncInterval = m_VsyncInterval;
  MTX_CHECK(m_interface.CreateSwapChain(m_interface.getDevice(), swapDesc, m_swapChain));
  uint32_t             swapchainFrameNum = 0;
  nri::Texture* const* swapchainTextures =
      m_interface.GetSwapChainTextures(*m_swapChain, swapchainFrameNum);
  format = m_interface.GetTextureDesc(*swapchainTextures[0]).format;
  for (int i = 0; i < swapchainFrameNum; ++i) {
    m_frameResource[i]._frameTexture.tex = swapchainTextures[i];
    nri::Texture2DViewDesc texViewDesc{};
    texViewDesc.texture = m_frameResource[i]._frameTexture.tex;
    texViewDesc.viewType = nri::Texture2DViewType::COLOR_ATTACHMENT;
    texViewDesc.format = format;
    MTX_CHECK(
        m_interface.CreateTexture2DView(texViewDesc, m_frameResource[i]._frameTexture.imageView));
  }
}

void MTXRtRenderer::PrepareFrame(uint32_t frameIndex) { std::cout << "test" << std::endl; }

void MTXRtRenderer::RenderFrame(uint32_t frameIndex) {}
}// namespace MTX