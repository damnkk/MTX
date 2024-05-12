#include "renderer.h"
#include "log.h"
#include "mtxUtils.h"
#include "sceneGraph.h"
#include "sceneLoader.h"

namespace MTX {
const int CMDBUFFER_CNT_PER_THREAD = 3;

MTXRenderer::~MTXRenderer() {
  m_interface.WaitForIdle(m_interface.getGraphicQueue());
  m_interface.WaitForIdle(m_interface.getComputeQueue());
  m_interface.WaitForIdle(m_interface.getTransferQueue());
}

bool MTXRenderer::Initialize(nri::GraphicsAPI graphicsAPI) {

  nri::AdapterDesc bestAdaptDesc = {};
  uint32_t         adapterDescsNum = 1;
  auto             res = nri::nriEnumerateAdapters(&bestAdaptDesc, adapterDescsNum);
  MTX_CHECK(res);
  nri::DeviceCreationDesc deviceCreationDesc = {};
  deviceCreationDesc.adapterDesc = nullptr;
  deviceCreationDesc.memoryAllocatorInterface = m_MemoryAllocatorInterface;
  deviceCreationDesc.graphicsAPI = nri::GraphicsAPI::VULKAN;
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

  MTX_CHECK(m_interface.GetCommandQueue(m_interface.getDevice(), nri::CommandQueueType::GRAPHICS,
                                        m_interface._graphicQueue));
  MTX_CHECK(m_interface.GetCommandQueue(m_interface.getDevice(), nri::CommandQueueType::COMPUTE,
                                        m_interface._computeQueue));
  MTX_CHECK(m_interface.GetCommandQueue(m_interface.getDevice(), nri::CommandQueueType::COPY,
                                        m_interface._transferQueue));
  m_interface.CreateCommandAllocator(m_interface.getGraphicQueue(),
                                     m_interface._instantCommandAllocator);
  m_frameResource.resize(MTX_MAX_FRAME_COUNT);
  for (auto& i : m_frameResource) {
    MTX_CHECK(m_interface.CreateFence(m_interface.getDevice(), 0, i._fence));
  }
  createCommandBuffer();
  nri::Format swpFormat = nri::Format::UNKNOWN;
  createSwapChain(swpFormat);
  m_sceneLoader = std::make_shared<SceneLoader>(&m_interface);
  m_SceneFile = "./Asset/models/MetalRoughSpheres/MetalRoughSpheres.gltf";
  m_sceneLoader->loadScene(m_SceneFile);
  createBLAS();
  createTLAS();
  createSBT();
  createRayTracingPipeline();

  MTX_INFO("----MTXRenderer initialized successfully-----")
  return InitUI(m_interface, m_interface, m_interface.getDevice(), swpFormat);
}

void MTXRenderer::createCommandBuffer() {
  for (auto& i : m_frameResource) {
    i._commandPools.resize(m_threadNum);
    i._commandBuffers.resize(m_threadNum * CMDBUFFER_CNT_PER_THREAD);
  }
  for (auto& i : m_frameResource) {
    for (auto& _pool : i._commandPools) {
      MTX_CHECK(m_interface.CreateCommandAllocator(m_interface.getGraphicQueue(), _pool));
    }
    for (int cmdIdx = 0; cmdIdx < i._commandBuffers.size(); ++cmdIdx) {
      int poolIdx = int(cmdIdx / CMDBUFFER_CNT_PER_THREAD);
      MTX_CHECK(
          m_interface.CreateCommandBuffer(*(i._commandPools[poolIdx]), i._commandBuffers[cmdIdx]));
    }
  }
}

void MTXRenderer::createSwapChain(nri::Format& format) {
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

void createRayTracingPipeline() {}

void MTXRenderer::createDescriptorSets() {
  nri::DescriptorPoolDesc desc{};
  desc.storageTextureMaxNum = 2;
  desc.accelerationStructureMaxNum = 4096;
  desc.bufferMaxNum = 4096;
  desc.descriptorSetMaxNum = 4096;
  MTX_CHECK(m_interface.CreateDescriptorPool(m_interface.getDevice(), desc, m_descriptorPool));
}

void MTXRenderer::createBLAS() {

  //prepare origin geometry data
  MtxBufferAllocInfo vertGeomACInfo{};
  vertGeomACInfo._desc.size = sizeof(Vertex) * m_sceneLoader->getVertices().size();
  vertGeomACInfo._desc.usageMask = nri::BufferUsageBits::ACCELERATION_STRUCTURE_BUILD_READ;
  vertGeomACInfo._data = m_sceneLoader->getVertices().data();
  auto vertexData = m_interface.allocateBuffer(vertGeomACInfo);

  MtxBufferAllocInfo indexGeomACInfo{};
  indexGeomACInfo._desc.size = sizeof(u32) * m_sceneLoader->getIndices().size();
  indexGeomACInfo._desc.usageMask = nri::BufferUsageBits::ACCELERATION_STRUCTURE_BUILD_READ;
  indexGeomACInfo._data = m_sceneLoader->getIndices().data();
  auto                             indexData = m_interface.allocateBuffer(indexGeomACInfo);
  uint64_t                         scratchBufferSize = 0;
  std::vector<nri::GeometryObject> geomObjects;
  geomObjects.reserve(m_sceneLoader->getMeshes().size());
  for (auto& i : m_sceneLoader->getMeshes()) {
    nri::GeometryObject geomObject{};
    geomObject.type = nri::GeometryType::TRIANGLES;
    geomObject.flags = nri::BottomLevelGeometryBits::NO_DUPLICATE_ANY_HIT_INVOCATION;
    geomObject.triangles.vertexBuffer = vertexData->buf;
    geomObject.triangles.vertexOffset = i.vertexOffset;
    geomObject.triangles.indexBuffer = indexData->buf;
    geomObject.triangles.indexOffset = i.indexOffset;
    geomObject.triangles.vertexFormat = nri::Format::RGB32_SFLOAT;
    geomObject.triangles.indexNum = i.indexCount;
    geomObject.triangles.indexType = nri::IndexType::UINT32;
    geomObject.triangles.vertexNum = i.vertexCount;
    geomObject.triangles.vertexStride = sizeof(Vertex);

    nri::AccelerationStructureDesc blasDesc{};
    blasDesc.flags = nri::AccelerationStructureBuildBits::PREFER_FAST_TRACE;
    blasDesc.type = nri::AccelerationStructureType::BOTTOM_LEVEL;
    blasDesc.geometryObjects = &geomObject;
    blasDesc.instanceOrGeometryObjectNum = 1;
    m_blas.emplace_back();
    auto blas = m_interface.allocateAccStructure(blasDesc);
    m_blas.push_back(blas);
    geomObjects.push_back(geomObject);
    scratchBufferSize =
        std::max(scratchBufferSize,
                 m_interface.GetAccelerationStructureBuildScratchBufferSize(*(blas->acc)));
  }
  //build blas
  MtxBufferAllocInfo scratchAllocInfo{};
  scratchAllocInfo._desc.size = scratchBufferSize;
  scratchAllocInfo._desc.usageMask = nri::BufferUsageBits::RAY_TRACING_BUFFER;
  auto scratchBuffer = m_interface.allocateBuffer(scratchAllocInfo);
  for (int i = 0; i < m_blas.size(); ++i) {
    auto& geomObject = geomObjects[i];
    auto  instantCmdBuf = m_interface.getInstantCommandBuffer();
    m_interface.CmdBuildBottomLevelAccelerationStructure(
        *instantCmdBuf, 1, &geomObject, nri::AccelerationStructureBuildBits::PREFER_FAST_TRACE,
        *(m_blas[i]->acc), scratchBuffer->getBuf(), 0);
    m_interface.EndCommandBuffer(*instantCmdBuf);
    nri::QueueSubmitDesc queueSubmitDesc{};
    queueSubmitDesc.commandBufferNum = 1;
    queueSubmitDesc.commandBuffers = &instantCmdBuf;
    m_interface.QueueSubmit(m_interface.getGraphicQueue(), queueSubmitDesc);
    m_interface.WaitForIdle(m_interface.getGraphicQueue());
  }
  //缺少资源释放函数,整理一条链路出来
  m_interface.m_bufferAllocator->releaseBuffer(vertexData->_uid);
  m_interface.m_bufferAllocator->releaseBuffer(indexData->_uid);
  m_interface.m_bufferAllocator->releaseBuffer(scratchBuffer->_uid);
}

inline void toTransformMatrixKHR(glm::mat4                                         matrix,
                                 decltype(nri::GeometryObjectInstance::transform)& transForm) {
  glm::mat4 temp = glm::transpose(matrix);
  memcpy(&transForm, &temp, sizeof(float) * 12);
  return;
}

void MTXRenderer::createTLAS() {
  auto sceneGraph = m_sceneLoader->getSceneGraph();

  nri::AccelerationStructureDesc tlasDesc{};
  tlasDesc.type = nri::AccelerationStructureType::TOP_LEVEL;
  tlasDesc.flags = nri::AccelerationStructureBuildBits::ALLOW_COMPACTION
      | nri::AccelerationStructureBuildBits::ALLOW_UPDATE
      | nri::AccelerationStructureBuildBits::PREFER_FAST_TRACE;
  tlasDesc.instanceOrGeometryObjectNum = sceneGraph->m_meshMap.size();
  m_tlas = m_interface.allocateAccStructure(tlasDesc);
  std::vector<nri::GeometryObjectInstance> geometryInstances(sceneGraph->m_meshMap.size(),
                                                             nri::GeometryObjectInstance{});
  //再次整理,tlas是blas的索引,blas是每个实际网格数据对应有的东西,但是一个blas带上不同的变换,会得到多个tlas
  //sceneGraph的meshMap是<节点ID,网格索引>
  int instanceCnt = 0;
  for (auto& mesh : sceneGraph->m_meshMap) {
    nri::GeometryObjectInstance& instance = geometryInstances[instanceCnt];
    instance.accelerationStructureHandle =
        m_interface.GetAccelerationStructureHandle(*(m_blas[mesh.second]->acc));
    instance.instanceId = instanceCnt;
    glm::mat4 globalTrans = sceneGraph->getGlobalTransformsFromIdx(mesh.first);
    toTransformMatrixKHR(globalTrans, instance.transform);
    instance.mask = 0xff;
    ++instanceCnt;
  }
  MtxBufferAllocInfo allocInfo{};
  allocInfo._memLocation = nri::MemoryLocation::HOST_UPLOAD;
  allocInfo._desc = {.size = geometryInstances.size() * sizeof(nri::GeometryObjectInstance),
                     .usageMask = nri::BufferUsageBits::ACCELERATION_STRUCTURE_BUILD_READ};
  auto  instanceBuffer = m_interface.allocateBuffer(allocInfo);
  void* data = m_interface.MapBuffer(instanceBuffer->getBuf(), 0, nri::WHOLE_SIZE);
  memcpy(data, geometryInstances.data(),
         geometryInstances.size() * sizeof(nri::GeometryObjectInstance));
  m_interface.UnmapBuffer(instanceBuffer->getBuf());

  MtxBufferAllocInfo scratchBufferAllocInfo{};
  scratchBufferAllocInfo._desc.size =
      m_interface.GetAccelerationStructureBuildScratchBufferSize(*(m_tlas->acc));
  scratchBufferAllocInfo._desc.usageMask = nri::BufferUsageBits::RAY_TRACING_BUFFER;
  auto scratchBuffer = m_interface.allocateBuffer(scratchBufferAllocInfo);
  auto cmd = m_interface.getInstantCommandBuffer();
  m_interface.CmdBuildTopLevelAccelerationStructure(
      *cmd, instanceCnt, instanceBuffer->getBuf(), 0,
      nri::AccelerationStructureBuildBits::PREFER_FAST_TRACE, *(m_tlas->acc),
      scratchBuffer->getBuf(), 0);
  m_interface.flushInstantCommandBuffer(cmd);
  m_interface.m_bufferAllocator->releaseBuffer(scratchBuffer->_uid);
  m_interface.m_bufferAllocator->releaseBuffer(instanceBuffer);
  m_interface.CreateAccelerationStructureDescriptor(*(m_tlas->acc), m_tlasDescriptor);
  nri::DescriptorRangeUpdateDesc updateDesc{&m_tlasDescriptor, 1, 0};
  m_interface.UpdateDescriptorRanges(*m_tlasDescriptorSet, 1, 1, &updateDesc);
}

void MTXRenderer::PrepareFrame(uint32_t frameIndex) {}

void MTXRenderer::RenderFrame(uint32_t frameIndex) {}
}// namespace MTX