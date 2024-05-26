#include "renderer.h"
#include "log.h"
#include "mtxUtils.h"
#include "sceneGraph.h"
#include "sceneLoader.h"

namespace MTX {
const int CMDBUFFER_CNT_PER_THREAD = 3;
struct mtxDebugAllocator {
  std::atomic_uint64_t allocationNum = 0;
  std::atomic_size_t   allocatedSize = 0;
};

MTXRenderer::~MTXRenderer() {
  // mtxDebugAllocator* debugAllocator = (mtxDebugAllocator*) m_MemoryAllocatorInterface.userArg;
  m_interface.WaitForIdle(m_interface.getGraphicQueue());
  m_interface.WaitForIdle(m_interface.getComputeQueue());
  m_interface.WaitForIdle(m_interface.getTransferQueue());
  for (auto& frameRes : m_frameResource) { frameRes.destroy(&m_interface); }
  m_interface.DestroySwapChain(*m_swapChain);
  m_interface.DestroyDescriptorPool(*m_descriptorPool);

  m_interface.destroy();

  DestroyUI(m_interface);
  nri::nriDestroyDevice(m_interface.getDevice());
  MTX_INFO("Renderering closed successfully")
}

bool MTXRenderer::Initialize(nri::GraphicsAPI graphicsAPI) {
  initCamera();
  // mtxDebugAllocator* debugAllocator = (mtxDebugAllocator*) m_MemoryAllocatorInterface.userArg;
  nri::AdapterDesc bestAdaptDesc = {};
  uint32_t         adapterDescsNum = 1;
  auto             res = nri::nriEnumerateAdapters(&bestAdaptDesc, adapterDescsNum);
  MTX_CHECK(res);
  nri::DeviceCreationDesc deviceCreationDesc = {};
  deviceCreationDesc.graphicsAPI = nri::GraphicsAPI::VULKAN;
  deviceCreationDesc.enableAPIValidation = true;
  deviceCreationDesc.enableNRIValidation = false;
  deviceCreationDesc.spirvBindingOffsets = SPIRV_BINDING_OFFSETS;
  deviceCreationDesc.adapterDesc = nullptr;
  deviceCreationDesc.memoryAllocatorInterface = m_MemoryAllocatorInterface;
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
    i._commandPools.resize(m_threadNum);
    i._commandBuffers.resize(m_threadNum * CMDBUFFER_CNT_PER_THREAD);
    for (auto& _pool : i._commandPools) {
      MTX_CHECK(m_interface.CreateCommandAllocator(m_interface.getGraphicQueue(), _pool));
    }
    for (int cmdIdx = 0; cmdIdx < i._commandBuffers.size(); ++cmdIdx) {
      int poolIdx = int(cmdIdx / CMDBUFFER_CNT_PER_THREAD);
      MTX_CHECK(
          m_interface.CreateCommandBuffer(*(i._commandPools[poolIdx]), i._commandBuffers[cmdIdx]));
    }
  }
  nri::Format swpFormat = nri::Format::RGBA8_SNORM;
  createSwapChain(swpFormat);
  m_sceneLoader = std::make_shared<SceneLoader>(&m_interface);
  m_SceneFile = "./Asset/models/DamagedHelmet/DamagedHelmet.gltf";
  //loadScene 泄露两个
  m_sceneLoader->loadScene(m_SceneFile);
  createRayTracingPipeline();
  createDescriptorSets();
  createBLAS();
  createTLAS();
  createSBT();
  updateDescriptorSets();
  MTX_INFO("----MTXRenderer initialized successfully-----")
  bool uiRes = InitUI(m_interface, m_interface, m_interface.getDevice(), swpFormat);
  return true;
}

void MTXRenderer::initCamera() {
  CameraDesc desc;
  desc.aspectRatio = (float) m_WindowResolution.x / (float) m_WindowResolution.y;
  m_Camera.Initialize(float3(0.0), float3(0.0, 0.0, -1.0f));
  GetCameraDescFromInputDevices(desc);
  m_Camera.Update(desc, 0);
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

void MTXRenderer::createRayTracingPipeline() {
  MtxPipelineAllocateInfo     pipelineAllocInfo{};
  nri::RayTracingPipelineDesc pipelineDesc{};
  nri::DescriptorRangeDesc    rangedescs[] = {
      //set0
      {0, 1, nri::DescriptorType::STORAGE_TEXTURE, nri::StageBits::RAYGEN_SHADER, false, false},
      {1, 1, nri::DescriptorType::ACCELERATION_STRUCTURE, nri::StageBits::RAYGEN_SHADER, false,
          false},
      //set1
      {0, 1, nri::DescriptorType::STORAGE_BUFFER, nri::StageBits::RAY_TRACING_SHADERS, false,
          false},
      {1, 1, nri::DescriptorType::STORAGE_BUFFER, nri::StageBits::CLOSEST_HIT_SHADER, false, false},
      {2, 1, nri::DescriptorType::STORAGE_BUFFER, nri::StageBits::CLOSEST_HIT_SHADER, false, false},
      {3, 1, nri::DescriptorType::STORAGE_BUFFER, nri::StageBits::CLOSEST_HIT_SHADER, false, false},
      //set2
      {0, static_cast<uint32_t>(m_sceneLoader->getSceneTextures().size()),
          nri::DescriptorType::TEXTURE, nri::StageBits::CLOSEST_HIT_SHADER,
          nri::VARIABLE_DESCRIPTOR_NUM, nri::DESCRIPTOR_ARRAY},
      //set3
      {0, static_cast<uint32_t>(m_sceneLoader->getMeshes().size()),
          nri::DescriptorType::STORAGE_BUFFER, nri::StageBits::CLOSEST_HIT_SHADER, false,
          nri::DESCRIPTOR_ARRAY},
  };

  std::vector<nri::DescriptorSetDesc> descs = {{0, rangedescs, 2},
                                               {1, rangedescs + 2, 4},
                                               {2, rangedescs + 6, 1},
                                               {3, rangedescs + 7, 1}};

  nri::PushConstantDesc pushConstDesc{0, sizeof(MtxRayTracingPushConstant),
                                      nri::StageBits::RAY_TRACING_SHADERS};

  nri::PipelineLayoutDesc pipelineLayoutDesc = {};
  pipelineLayoutDesc.descriptorSets = descs.data();
  pipelineLayoutDesc.descriptorSetNum = descs.size();
  pipelineLayoutDesc.shaderStages = nri::StageBits::RAY_TRACING_SHADERS;
  pipelineLayoutDesc.pushConstantNum = 1;
  pipelineLayoutDesc.pushConstants = &pushConstDesc;
  nri::PipelineLayout* layout;

  MTX_CHECK(m_interface.CreatePipelineLayout(m_interface.getDevice(), pipelineLayoutDesc, layout));
  ::utils::ShaderCodeStorage   shaderCodeStorage;
  std::vector<nri::ShaderDesc> shaders = {
      ::utils::LoadShader(m_interface.GetDeviceDesc(m_interface.getDevice()).graphicsAPI,
                          "RayTracingBox.rgen", shaderCodeStorage, "raygen"),
      ::utils::LoadShader(m_interface.GetDeviceDesc(m_interface.getDevice()).graphicsAPI,
                          "RayTracingBox.rmiss", shaderCodeStorage, "miss"),
      ::utils::LoadShader(m_interface.GetDeviceDesc(m_interface.getDevice()).graphicsAPI,
                          "RayTracingBox.rchit", shaderCodeStorage, "closest_hit"),
  };

  nri::ShaderLibrary shaderLib = {};
  shaderLib.shaders = shaders.data();
  shaderLib.shaderNum = shaders.size();

  nri::ShaderGroupDesc groupDesc[] = {{1}, {2}, {3}};

  pipelineDesc.recursionDepthMax = 8;
  pipelineDesc.payloadAttributeSizeMax = 128;
  pipelineDesc.intersectionAttributeSizeMax = 128;
  pipelineDesc.pipelineLayout = layout;
  pipelineDesc.shaderGroupDescs = groupDesc;
  pipelineDesc.shaderGroupDescNum = helper::GetCountOf(groupDesc);
  pipelineDesc.shaderLibrary = &shaderLib;
  pipelineAllocInfo.pipelineDesc = &pipelineDesc;
  pipelineAllocInfo.pipelineType = PipelineType::RayTracing;
  pipelineAllocInfo.name = "RayTracingName";
  m_rayTracingPipeline = m_interface.allocatePipeline(pipelineAllocInfo);
}

void MTXRenderer::createDescriptorSets() {
  nri::DescriptorPoolDesc desc{};
  desc.storageTextureMaxNum = MTX_MAX_FRAME_COUNT;
  desc.accelerationStructureMaxNum = 4096;
  desc.bufferMaxNum = 1024;
  desc.textureMaxNum = 1024;
  desc.descriptorSetMaxNum = 4096;
  desc.storageBufferMaxNum = 4096;
  m_descriptorSets.resize(4);
  MTX_CHECK(m_interface.CreateDescriptorPool(m_interface.getDevice(), desc, m_descriptorPool));
  MTX_CHECK(m_interface.AllocateDescriptorSets(
      *m_descriptorPool, *(m_rayTracingPipeline->pipelineLayout), 0, &m_descriptorSets[0], 1, 0));
  MTX_CHECK(m_interface.AllocateDescriptorSets(
      *m_descriptorPool, *(m_rayTracingPipeline->pipelineLayout), 1, &m_descriptorSets[1], 1, 0));
  MTX_CHECK(m_interface.AllocateDescriptorSets(
      *m_descriptorPool, *(m_rayTracingPipeline->pipelineLayout), 2, &m_descriptorSets[2], 1,
      m_sceneLoader->getSceneTextures().size()));
  MTX_CHECK(m_interface.AllocateDescriptorSets(
      *m_descriptorPool, *(m_rayTracingPipeline->pipelineLayout), 3, &m_descriptorSets[3], 1,
      m_sceneLoader->getMeshes().size()));
}

void MTXRenderer::updateDescriptorSets() {
  std::vector<Vertex>&                   sceneVertices = m_sceneLoader->getVertices();
  std::vector<uint32_t>&                 sceneIndices = m_sceneLoader->getIndices();
  std::vector<Material::MaterialUniform> materials;
  for (auto& mat : m_sceneLoader->getMaterials()) {
    for (auto& bind : mat.m_textureMap) {
      mat.materialUniform.textureIndices[bind.second.bindIdx] = {bind.second.textureHandle};
    }
    materials.push_back(mat.materialUniform);
  }
  std::vector<std::vector<uint32_t>> primitiveIdxDatas;
  std::vector<RtInstanceInfo>        instanceInfo;
  int                                primitiveOffset = 0;
  uint32_t                           meshIdx = 0;
  for (auto& mesh : m_sceneLoader->getMeshes()) {
    std::vector<uint32_t> primData = m_sceneLoader->getPrimitMatIndices();
    std::vector<uint32_t> meshPrimData(primData.begin() + primitiveOffset,
                                       primData.begin() + primitiveOffset
                                           + uint32_t(mesh.indexCount / 3));
    // std::vector<uint32_t> meshPrimData(primData.begin() + primitiveOffset, primData.begin() + 500);
    primitiveIdxDatas.push_back(meshPrimData);
    instanceInfo.push_back({.indexOffset = mesh.indexOffset,
                            .vertexOffset = mesh.vertexOffset,
                            .vertexCount = mesh.vertexCount,
                            .indexCount = mesh.indexCount,
                            .meshIdx = meshIdx});
    ++meshIdx;
    primitiveOffset += (mesh.indexOffset % 3);
  }

  std::vector<std::shared_ptr<MtxTexture>> sceneTextures = m_sceneLoader->getSceneTextures();

  //upload vertex Buffer
  MtxBufferAllocInfo bufferAllocInfo{};
  bufferAllocInfo._desc.size = sizeof(Vertex) * sceneVertices.size();
  bufferAllocInfo._desc.usageMask = nri::BufferUsageBits::SHADER_RESOURCE_STORAGE;
  bufferAllocInfo._desc.structureStride = 0;
  bufferAllocInfo._name = "sceneVerticesData";
  bufferAllocInfo._data = sceneVertices.data();
  auto sceneVerticesBuffer = m_interface.allocateBuffer(bufferAllocInfo);

  //upload index Buffer
  bufferAllocInfo._desc.size = sizeof(uint32_t) * sceneIndices.size();
  bufferAllocInfo._name = "sceneIndicesData";
  bufferAllocInfo._data = sceneIndices.data();
  auto sceneIndicesBuffer = m_interface.allocateBuffer(bufferAllocInfo);

  //upload instanceInfo Buffer
  bufferAllocInfo._desc.size = sizeof(RtInstanceInfo) * instanceInfo.size();
  bufferAllocInfo._name = "instanceInfoData";
  bufferAllocInfo._data = sceneIndices.data();
  auto instanceInfoBuffer = m_interface.allocateBuffer(bufferAllocInfo);

  //upload matUniform Buffer
  bufferAllocInfo._desc.size = sizeof(Material::MaterialUniform) * materials.size();
  bufferAllocInfo._name = "materialData";
  bufferAllocInfo._data = materials.data();
  auto materialBuffer = m_interface.allocateBuffer(bufferAllocInfo);

  std::vector<std::shared_ptr<MtxBuffer>> primitDatas;
  for (int i = 0; i < primitiveIdxDatas.size(); ++i) {
    bufferAllocInfo._desc.size = sizeof(uint32_t) * primitiveIdxDatas[i].size();
    bufferAllocInfo._name = "primitiveData" + std::to_string(i);
    bufferAllocInfo._data = primitiveIdxDatas[i].data();
    primitDatas.push_back(m_interface.allocateBuffer(bufferAllocInfo));
  }

  //update buffer descriptor
  nri::BufferViewDesc bufferViewDesc{};
  bufferViewDesc.buffer = sceneVerticesBuffer->buf;
  bufferViewDesc.viewType = nri::BufferViewType::SHADER_RESOURCE_STORAGE;
  bufferViewDesc.size = sceneVerticesBuffer->size();
  bufferViewDesc.offset = 0;
  m_interface.CreateBufferView(bufferViewDesc, sceneVerticesBuffer->bufView);

  bufferViewDesc.buffer = sceneIndicesBuffer->buf;
  bufferViewDesc.viewType = nri::BufferViewType::SHADER_RESOURCE_STORAGE;
  bufferViewDesc.size = sceneIndicesBuffer->size();
  bufferViewDesc.offset = 0;
  m_interface.CreateBufferView(bufferViewDesc, sceneIndicesBuffer->bufView);

  bufferViewDesc.buffer = instanceInfoBuffer->buf;
  bufferViewDesc.viewType = nri::BufferViewType::SHADER_RESOURCE_STORAGE;
  bufferViewDesc.size = instanceInfoBuffer->size();
  bufferViewDesc.offset = 0;
  m_interface.CreateBufferView(bufferViewDesc, instanceInfoBuffer->bufView);

  bufferViewDesc.buffer = materialBuffer->buf;
  bufferViewDesc.viewType = nri::BufferViewType::SHADER_RESOURCE_STORAGE;
  bufferViewDesc.size = materialBuffer->size();
  bufferViewDesc.offset = 0;
  m_interface.CreateBufferView(bufferViewDesc, materialBuffer->bufView);

  for (int i = 0; i < primitDatas.size(); ++i) {
    bufferViewDesc.buffer = primitDatas[i]->buf;
    bufferViewDesc.viewType = nri::BufferViewType::SHADER_RESOURCE_STORAGE;
    bufferViewDesc.size = primitDatas[i]->size();
    bufferViewDesc.offset = 0;
    m_interface.CreateBufferView(bufferViewDesc, primitDatas[i]->bufView);
  }

  //update texture  descriptor
  // bindless texture array
  for (int i = 0; i < sceneTextures.size(); ++i) {
    nri::Texture2DViewDesc desc{};
    desc.viewType = nri::Texture2DViewType::SHADER_RESOURCE_2D;
    desc.format = sceneTextures[i]->format();
    m_interface.CreateTexture2DView(desc, sceneTextures[i]->imageView);
  }

  nri::DescriptorRangeUpdateDesc rangeUpdateDesc = {};
  rangeUpdateDesc.descriptorNum = 1;
  for (uint32_t i = 0; i < sceneTextures.size(); ++i) {
    rangeUpdateDesc.offsetInRange = i;
    rangeUpdateDesc.descriptors = &(sceneTextures[i]->imageView);
    m_interface.UpdateDescriptorRanges(*m_descriptorSets[2], 0, 1, &rangeUpdateDesc);
  }
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
    m_interface.flushInstantCommandBuffer(instantCmdBuf);
    // nri::QueueSubmitDesc queueSubmitDesc{};
    // queueSubmitDesc.commandBufferNum = 1;
    // queueSubmitDesc.commandBuffers = &instantCmdBuf;
    // m_interface.QueueSubmit(m_interface.getGraphicQueue(), queueSubmitDesc);
    // m_interface.WaitForIdle(m_interface.getGraphicQueue());
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
  tlasDesc.flags = nri::AccelerationStructureBuildBits::PREFER_FAST_TRACE
      // |nri::AccelerationStructureBuildBits::ALLOW_UPDATE      //这个目前不支持
      | nri::AccelerationStructureBuildBits::ALLOW_COMPACTION;
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
  m_interface.CreateAccelerationStructureDescriptor(*(m_tlas->acc), m_tlas->accView);
  nri::DescriptorRangeUpdateDesc updateDesc{&m_tlas->accView, 1, 0};
  m_interface.UpdateDescriptorRanges(*m_descriptorSets[0], 1, 1, &updateDesc);
}

void MTXRenderer::createSBT() {
  const nri::DeviceDesc& deviceDesc = m_interface.GetDeviceDesc(m_interface.getDevice());
  const uint64_t         identifierSize = deviceDesc.rayTracingShaderGroupIdentifierSize;
  const uint64_t         tableAlignment = deviceDesc.rayTracingShaderTableAligment;

  m_shaderGroupIdentifierSize = identifierSize;
  m_missShaderOffset = helper::Align(identifierSize, tableAlignment);
  m_hitShaderGroupOffset = helper::Align(m_missShaderOffset + identifierSize, tableAlignment);
  const uint64_t SBTSize = helper::Align(m_hitShaderGroupOffset + identifierSize, tableAlignment);

  MtxBufferAllocInfo bufferInfo{};
  bufferInfo._desc.size = SBTSize;
  bufferInfo._desc.usageMask = nri::BufferUsageBits::RAY_TRACING_BUFFER;
  bufferInfo._name = "sbtBuffer";
  bufferInfo._memLocation = nri::MemoryLocation::DEVICE;
  m_shaderBindingTable = m_interface.allocateBuffer(bufferInfo);
  std::vector<uint8_t> content((size_t) SBTSize, 0);
  for (uint32_t i = 0; i < 3; ++i) {
    m_interface.WriteShaderGroupIdentifiers(
        m_rayTracingPipeline->getPipeline(), i, 1,
        content.data() + i * helper::Align(identifierSize, tableAlignment));
  }

  nri::BufferUploadDesc dataDesc = {};
  dataDesc.data = content.data();
  dataDesc.dataSize = content.size();
  dataDesc.buffer = m_shaderBindingTable->buf;
  dataDesc.after = {nri::AccessBits::UNKNOWN};

  MTX_CHECK(m_interface.UploadData(m_interface.getTransferQueue(), nullptr, 0, &dataDesc, 1));
}

void MTXRenderer::updateCamera(float deltaTime) {
  CameraDesc mainCameraDesc;
  // if (m_KeyState[(uint32_t) Key::D]) {
  //   int i = 0;
  //   std::cout << "true" << std::endl;
  // }
  GetCameraDescFromInputDevices(mainCameraDesc);
  m_Camera.Update(mainCameraDesc, m_frameIndex);
}

void MTXRenderer::PrepareFrame(uint32_t frameIndex) {
  // MTX_INFO("prepare frame");
  updateCamera(m_Timer.GetFrameTime());
}

void MTXRenderer::RenderFrame(uint32_t frameIndex) {
  // MTX_INFO("render frame");
}
}// namespace MTX