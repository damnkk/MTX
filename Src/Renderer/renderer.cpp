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
  mtxDebugAllocator* debugAllocator = (mtxDebugAllocator*) m_MemoryAllocatorInterface.userArg;
  m_interface.WaitForIdle(m_interface.getGraphicQueue());
  m_interface.WaitForIdle(m_interface.getComputeQueue());
  m_interface.WaitForIdle(m_interface.getTransferQueue());
  for (auto& frameRes : m_frameResource) { frameRes.destroy(&m_interface); }
  m_interface.DestroySwapChain(*m_swapChain);
  m_interface.DestroyDescriptorPool(*m_descriptorPool);
  m_interface.DestroyDescriptor(*m_sampler);
  m_interface.DestroyDescriptor(*m_rayTracingSampleView);
  m_interface.destroy();

  DestroyUI(m_interface);
  nri::nriDestroyDevice(m_interface.getDevice());
  MTX_INFO("Renderering closed successfully")
}

bool MTXRenderer::Initialize(nri::GraphicsAPI graphicsAPI) {
  // mtxDebugAllocator* debugAllocator = (mtxDebugAllocator*) m_MemoryAllocatorInterface.userArg;
  nri::AdapterDesc bestAdaptDesc = {};
  uint32_t         adapterDescsNum = 1;
  auto             res = nri::nriEnumerateAdapters(&bestAdaptDesc, adapterDescsNum);
  MTX_CHECK(res);
  nri::DeviceCreationDesc deviceCreationDesc = {};
  deviceCreationDesc.graphicsAPI = graphicsAPI;
  deviceCreationDesc.enableAPIValidation = false;
  deviceCreationDesc.enableNRIValidation = true;
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

  initCamera();
  nri::Format swpFormat = nri::Format::RGBA8_SNORM;
  createSwapChain(swpFormat);
  m_sceneLoader = std::make_shared<SceneLoader>(&m_interface);
  m_sceneLoader->addEnvTexture("E:/repository/MTX/Asset/hdrTex/daytime.hdr");
  // m_SceneFile = "./Asset/models/DamagedHelmet/DamagedHelmet.gltf";
  // m_sceneLoader->loadScene(m_SceneFile);
  m_SceneFile = "./Asset/models/MetalRoughSpheres/MetalRoughSpheres.gltf";
  m_sceneLoader->loadScene(m_SceneFile);
  createRayTracingPipeline();
  createPostProcessPipeline();
  createDescriptorSets();
  createRayTracingTex(nri::Format::RGBA32_SFLOAT);
  createBLAS();
  createTLAS();
  createSBT();
  updateDescriptorSets();
  MTX_INFO("----MTXRenderer initialized successfully-----")
  return InitUI(m_interface, m_interface, m_interface.getDevice(), swpFormat);
}

void MTXRenderer::createRayTracingTex(nri::Format fmt) {
  MtxTextureAllocInfo texInfo{};
  texInfo._name = "rayTracingTexture";
  texInfo._desc.width = (uint16_t) GetWindowResolution().x;
  texInfo._desc.height = (uint16_t) GetWindowResolution().y;
  texInfo._desc.type = nri::TextureType::TEXTURE_2D;
  texInfo._desc.arraySize = 1;
  texInfo._desc.mipNum = 1;
  texInfo._desc.sampleNum = 1;
  texInfo._desc.depth = 1;
  texInfo._desc.format = fmt;
  texInfo._desc.usageMask = nri::TextureUsageBits::SHADER_RESOURCE_STORAGE|nri::TextureUsageBits::SHADER_RESOURCE;

  m_rayTracingTexture = m_interface.allocateTexture(texInfo);
  nri::Texture2DViewDesc textureViewDesc = {m_rayTracingTexture->tex,
                                            nri::Texture2DViewType::SHADER_RESOURCE_STORAGE_2D,
                                            fmt,
                                            0,
                                            1,
                                            0,
                                            1};
  MTX_CHECK(m_interface.CreateTexture2DView(textureViewDesc, m_rayTracingTexture->imageView));
  const nri::DescriptorRangeUpdateDesc updateDesc{&(m_rayTracingTexture->imageView), 1, 0};
  m_interface.UpdateDescriptorRanges(*m_descriptorSets[0], 0, 1, &updateDesc);
  textureViewDesc.viewType = nri::Texture2DViewType::SHADER_RESOURCE_2D;
  MTX_CHECK(m_interface.CreateTexture2DView(textureViewDesc,m_rayTracingSampleView));
}

void MTXRenderer::initCamera() {
  CameraDesc desc;
  desc.aspectRatio = float(GetWindowResolution().x) / float(GetWindowResolution().y);
  desc.nearZ = 0.1f;
  m_cameras.emplace_back();
  m_cameras.front().Initialize(float3(0.0, 0.0, 0.0), float3(0.0, -1.0, 0.0f));
  MtxBufferAllocInfo camUnifoInfo{};
  camUnifoInfo._name = "cameraUniform";
  camUnifoInfo._desc.size = sizeof(CameraUniform);
  camUnifoInfo._desc.usageMask = nri::BufferUsageBits::SHADER_RESOURCE;
  camUnifoInfo._memLocation = nri::MemoryLocation::HOST_UPLOAD;
  m_cameras.front().camUniformBuffer = m_interface.allocateBuffer(camUnifoInfo);
  GetCameraDescFromInputDevices(desc);
  m_cameras.front().updateState(desc, 0, m_Timer.GetFrameTime());
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
  std::vector<nri::DescriptorRangeDesc> rangeDesc1 = {
     //set0 ---> rayTracing texture/ tlas / camera uniform
      {0, 1, nri::DescriptorType::STORAGE_TEXTURE, nri::StageBits::RAYGEN_SHADER, false, false},
      {1, 1, nri::DescriptorType::ACCELERATION_STRUCTURE, nri::StageBits::RAYGEN_SHADER, false,
          false},
      {2, 1, nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::RAY_TRACING_SHADERS, false,
          false}

  };
  std::vector<nri::DescriptorRangeDesc> rangeDesc2 = {
      //set1 ---> material uniform/ vertices/ indices/ instance info/textureSampler
      {0, 1, nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::RAY_TRACING_SHADERS, false,
          false},
      {1, 1, nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::CLOSEST_HIT_SHADER, false,
          false},
      {2, 1, nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::CLOSEST_HIT_SHADER, false,
          false},
      {3, 1, nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::CLOSEST_HIT_SHADER, false,
          false},
      {4, 1, nri::DescriptorType::SAMPLER, nri::StageBits::RAY_TRACING_SHADERS, false, false},
      {5, 1, nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::RAY_TRACING_SHADERS, false, false}
      };
  
  std::vector<nri::DescriptorRangeDesc> rangeDesc3 = {{
      //set2 ---> scene textures
      0, static_cast<uint32_t>(m_sceneLoader->getSceneTextures().size()),nri::DescriptorType::TEXTURE, nri::StageBits::CLOSEST_HIT_SHADER,
          nri::VARIABLE_DESCRIPTOR_NUM, nri::DESCRIPTOR_ARRAY}};
  std::vector<nri::DescriptorRangeDesc> rangeDesc4={
        //set3 ---> env textures
  {0, static_cast<uint32_t>(m_sceneLoader->getEnvTextures().size()),nri::DescriptorType::TEXTURE, nri::StageBits::MISS_SHADER, 
  nri::VARIABLE_DESCRIPTOR_NUM,nri::DESCRIPTOR_ARRAY}
  };
      
  std::vector<nri::DescriptorRangeDesc> rangeDesc5={
  //set4 ---> primitives info
  {0, static_cast<uint32_t>(m_sceneLoader->getMeshes().size()),nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::CLOSEST_HIT_SHADER,
      nri::VARIABLE_DESCRIPTOR_NUM, nri::DESCRIPTOR_ARRAY},
  };

  std::vector<nri::DescriptorSetDesc> descs = {{0, rangeDesc1.data(), (uint32_t)rangeDesc1.size()},
                                               {1, rangeDesc2.data(), (uint32_t)rangeDesc2.size()},
                                               {2, rangeDesc3.data(), (uint32_t)rangeDesc3.size()},
                                               {3, rangeDesc4.data(), (uint32_t)rangeDesc4.size()},
                                               {4, rangeDesc5.data(), (uint32_t)rangeDesc5.size()}};

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

  pipelineDesc.recursionDepthMax = m_maxBounce;
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

void MTXRenderer::createPostProcessPipeline(){
  MtxPipelineAllocateInfo postPipelineInfo{};
  nri::GraphicsPipelineDesc pipelineDesc{};
  nri::DescriptorRangeDesc rangeDescs[] ={
    {0,1,nri::DescriptorType::TEXTURE,nri::StageBits::FRAGMENT_SHADER,false,false},
    {1,1,nri::DescriptorType::SAMPLER,nri::StageBits::FRAGMENT_SHADER,false,false},
  };

  std::vector<nri::DescriptorSetDesc>descs = {
    {0,rangeDescs,helper::GetCountOf(rangeDescs)}
  };

  nri::PushConstantDesc pushDescs[] ={
    {0,sizeof(MtxPostProcessPushConstant),nri::StageBits::FRAGMENT_SHADER}
  };

  nri::PipelineLayoutDesc postLayoutDesc{};
  postLayoutDesc.descriptorSetNum = descs.size();
  postLayoutDesc.descriptorSets = descs.data();
  postLayoutDesc.pushConstantNum = helper::GetCountOf(pushDescs);
  postLayoutDesc.pushConstants = pushDescs;
  postLayoutDesc.shaderStages = nri::StageBits::GRAPHICS_SHADERS;
  nri::PipelineLayout* layout;
  MTX_CHECK(m_interface.CreatePipelineLayout(m_interface.getDevice(),postLayoutDesc,layout));
  ::utils::ShaderCodeStorage shaderCodeStorage;
  std::vector<nri::ShaderDesc> shaders ={
    ::utils::LoadShader(m_interface.GetDeviceDesc(m_interface.getDevice()).graphicsAPI,
     "postProcess.vs", shaderCodeStorage,"main"),
     ::utils::LoadShader(m_interface.GetDeviceDesc(m_interface.getDevice()).graphicsAPI, 
     "postProcess.fs", shaderCodeStorage,"main")
  };

  nri::InputAssemblyDesc inputAssemblyDesc ={};
  inputAssemblyDesc.topology = nri::Topology::TRIANGLE_LIST;

  nri::RasterizationDesc rasterizationDesc = {};
  rasterizationDesc.viewportNum = 1;
  rasterizationDesc.fillMode = nri::FillMode::SOLID;
  rasterizationDesc.cullMode = nri::CullMode::NONE;

  nri::ColorAttachmentDesc colorAttachmentDesc = {};
  colorAttachmentDesc.format = nri::Format::RGBA8_SNORM;
  colorAttachmentDesc.colorWriteMask = nri::ColorWriteBits::RGBA;
  colorAttachmentDesc.blendEnabled = false;

  nri::OutputMergerDesc outputMergerDesc = {};
  outputMergerDesc.colorNum = 1;
  outputMergerDesc.color=  &colorAttachmentDesc;
  

  pipelineDesc.pipelineLayout = layout;
  pipelineDesc.vertexInput = nullptr;
  pipelineDesc.inputAssembly = inputAssemblyDesc;
  pipelineDesc.rasterization = rasterizationDesc;
  pipelineDesc.outputMerger = outputMergerDesc;
  pipelineDesc.shaders = shaders.data();
  pipelineDesc.shaderNum = shaders.size();
  postPipelineInfo.pipelineDesc = &pipelineDesc;
  postPipelineInfo.pipelineType = PipelineType::Graphics;
  postPipelineInfo.name = "postProcessPipeline";
  m_postProcessPipeline = m_interface.allocatePipeline(postPipelineInfo);
}

void MTXRenderer::createDescriptorSets() {
  nri::DescriptorPoolDesc desc{};
  desc.storageTextureMaxNum = MTX_MAX_FRAME_COUNT;
  desc.accelerationStructureMaxNum = 4096;
  desc.samplerMaxNum = 8;
  desc.bufferMaxNum = 1024;
  desc.textureMaxNum = 1024;
  desc.structuredBufferMaxNum = 1024;
  desc.descriptorSetMaxNum = 4096;
  desc.storageBufferMaxNum = 4096;
  m_descriptorSets.resize(5);
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
      m_sceneLoader->getEnvTextures().size()));
  MTX_CHECK(m_interface.AllocateDescriptorSets(
      *m_descriptorPool, *(m_rayTracingPipeline->pipelineLayout), 4, &m_descriptorSets[4], 1,
      m_sceneLoader->getMeshes().size()));
  //----------------------------------------------------------------------------------------------
  m_postDescriptorSets.resize(1);
  MTX_CHECK(m_interface.AllocateDescriptorSets(
    *m_descriptorPool,*(m_postProcessPipeline->pipelineLayout),0,&m_postDescriptorSets[0], 1, 0));
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
    // std::vector<uint32_t> meshPrimData(uint32_t(mesh.indexCount / 3), 1);
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
  std::vector<std::shared_ptr<MtxTexture>> envTextures = m_sceneLoader->getEnvTextures();

  //upload vertex Buffer
  MtxBufferAllocInfo bufferAllocInfo{};
  bufferAllocInfo._desc.size = sizeof(Vertex) * sceneVertices.size();
  bufferAllocInfo._desc.usageMask = nri::BufferUsageBits::SHADER_RESOURCE;
  bufferAllocInfo._desc.structureStride = sizeof(Vertex);
  bufferAllocInfo._name = "sceneVerticesData";
  bufferAllocInfo._data = sceneVertices.data();
  auto sceneVerticesBuffer = m_interface.allocateBuffer(bufferAllocInfo);

  //upload index Buffer
  bufferAllocInfo._desc.size = sizeof(uint32_t) * sceneIndices.size();
  bufferAllocInfo._desc.usageMask = nri::BufferUsageBits::SHADER_RESOURCE;
  bufferAllocInfo._desc.structureStride = sizeof(uint32_t);
  bufferAllocInfo._name = "sceneIndicesData";
  bufferAllocInfo._data = sceneIndices.data();
  auto sceneIndicesBuffer = m_interface.allocateBuffer(bufferAllocInfo);

  //upload instanceInfo Buffer
  bufferAllocInfo._desc.size = sizeof(RtInstanceInfo) * instanceInfo.size();
  bufferAllocInfo._desc.structureStride = sizeof(RtInstanceInfo);
  bufferAllocInfo._desc.usageMask = nri::BufferUsageBits::SHADER_RESOURCE;
  bufferAllocInfo._name = "instanceInfoData";
  bufferAllocInfo._data = instanceInfo.data();
  auto instanceInfoBuffer = m_interface.allocateBuffer(bufferAllocInfo);

  //upload matUniform Buffer
  bufferAllocInfo._desc.size = sizeof(Material::MaterialUniform) * materials.size();
  bufferAllocInfo._desc.structureStride = sizeof(Material::MaterialUniform);
  bufferAllocInfo._desc.usageMask = nri::BufferUsageBits::SHADER_RESOURCE;
  bufferAllocInfo._name = "materialData";
  bufferAllocInfo._data = materials.data();
  auto materialBuffer = m_interface.allocateBuffer(bufferAllocInfo);

  std::vector<std::shared_ptr<MtxBuffer>> primitDatas;
  for (int i = 0; i < primitiveIdxDatas.size(); ++i) {
    bufferAllocInfo._desc.size = sizeof(uint32_t) * primitiveIdxDatas[i].size();
    bufferAllocInfo._desc.structureStride = 0;
    bufferAllocInfo._desc.usageMask = nri::BufferUsageBits::SHADER_RESOURCE_STORAGE;
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

  bufferViewDesc.buffer = m_cameras.front().camUniformBuffer->buf;
  bufferViewDesc.viewType = nri::BufferViewType::SHADER_RESOURCE;
  bufferViewDesc.size = m_cameras.front().camUniformBuffer->size();
  bufferViewDesc.offset = 0;
  m_interface.CreateBufferView(bufferViewDesc, m_cameras.front().camUniformBuffer->bufView);

  bufferViewDesc.buffer = m_sceneLoader->getEnvPdfBuffer()[0]->buf;
  bufferViewDesc.viewType = nri::BufferViewType::SHADER_RESOURCE_STORAGE;
  bufferViewDesc.size = m_sceneLoader->getEnvPdfBuffer()[0]->size();
  bufferViewDesc.offset = 0;
  m_interface.CreateBufferView(bufferViewDesc, m_sceneLoader->getEnvPdfBuffer()[0]->bufView);

  nri::SamplerDesc samplerDesc{};
  samplerDesc.addressModes.u = nri::AddressMode::REPEAT;
  samplerDesc.addressModes.v = nri::AddressMode::REPEAT;
  samplerDesc.anisotropy = 1;
  samplerDesc.filters.min = nri::Filter::LINEAR;
  samplerDesc.filters.mag = nri::Filter::LINEAR;
  samplerDesc.filters.mip = nri::Filter::LINEAR;
  samplerDesc.mipMax = 16.0f;
  MTX_CHECK(m_interface.CreateSampler(m_interface.getDevice(), samplerDesc, m_sampler));

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
    desc.texture = sceneTextures[i]->tex;
    m_interface.CreateTexture2DView(desc, sceneTextures[i]->imageView);
  }

  for (int i = 0; i < envTextures.size(); ++i) {
    nri::Texture2DViewDesc desc{};
    desc.viewType = nri::Texture2DViewType::SHADER_RESOURCE_2D;
    desc.format = envTextures[i]->format();
    desc.texture = envTextures[i]->tex;
    m_interface.CreateTexture2DView(desc, envTextures[i]->imageView);
  }

  nri::DescriptorRangeUpdateDesc rangeUpdateDesc = {};
  rangeUpdateDesc.descriptorNum = 1;
  for (uint32_t i = 0; i < sceneTextures.size(); ++i) {
    rangeUpdateDesc.offsetInRange = i;
    rangeUpdateDesc.descriptors = &(sceneTextures[i]->imageView);
    m_interface.UpdateDescriptorRanges(*m_descriptorSets[2], 0, 1, &rangeUpdateDesc);
  }

  for (uint32_t i = 0; i < envTextures.size(); ++i) {
    rangeUpdateDesc.offsetInRange = i;
    rangeUpdateDesc.descriptors = &(envTextures[i]->imageView);
    m_interface.UpdateDescriptorRanges(*m_descriptorSets[3], 0, 1, &rangeUpdateDesc);
  }

  // update buffer descriptor
  // bindless buffer array
  for (int i = 0; i < m_sceneLoader->getMeshes().size(); ++i) {
    rangeUpdateDesc.offsetInRange = i;
    rangeUpdateDesc.descriptors = &(primitDatas[i]->bufView);
    m_interface.UpdateDescriptorRanges(*m_descriptorSets[4], 0, 1, &rangeUpdateDesc);
  }
  auto& envPdfBuffer = m_sceneLoader->getEnvPdfBuffer().front();
  nri::Descriptor* bufferRanges[6] = {materialBuffer->bufView, sceneVerticesBuffer->bufView,
                                      sceneIndicesBuffer->bufView, instanceInfoBuffer->bufView,
                                      m_sampler, envPdfBuffer->bufView};
  nri::DescriptorRangeUpdateDesc bufferRangeUpdateDesc = {};
  for (int i = 0; i < helper::GetCountOf(bufferRanges); ++i) {
    bufferRangeUpdateDesc.descriptorNum = 1;
    bufferRangeUpdateDesc.descriptors = bufferRanges + i;
    bufferRangeUpdateDesc.offsetInRange = 0;
    m_interface.UpdateDescriptorRanges(*m_descriptorSets[1], i, 1, &bufferRangeUpdateDesc);
  }

  bufferRangeUpdateDesc.descriptorNum = 1;
  bufferRangeUpdateDesc.descriptors = &(m_cameras.front().camUniformBuffer->bufView);
  bufferRangeUpdateDesc.offsetInRange = 0;
  m_interface.UpdateDescriptorRanges(*m_descriptorSets[0], 2, 1, &bufferRangeUpdateDesc);
  /*-------------------update post descriptor----------------------------*/
  nri::Descriptor* postRanges[2] = {m_rayTracingSampleView,m_sampler};
  for(int i = 0;i<helper::GetCountOf(postRanges);++i){
    bufferRangeUpdateDesc.descriptorNum =1;
    bufferRangeUpdateDesc.descriptors = postRanges+i;
    bufferRangeUpdateDesc.offsetInRange = 0;
    m_interface.UpdateDescriptorRanges(*m_postDescriptorSets[0],i,1,&bufferRangeUpdateDesc);
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
  int meshIdx = 0;
  for (auto& i : m_sceneLoader->getMeshes()) {
    nri::GeometryObject geomObject{};
    geomObject.type = nri::GeometryType::TRIANGLES;
    geomObject.flags = nri::BottomLevelGeometryBits::NO_DUPLICATE_ANY_HIT_INVOCATION;
    geomObject.triangles.vertexBuffer = vertexData->buf;
    geomObject.triangles.vertexOffset = i.vertexOffset * sizeof(Vertex);
    geomObject.triangles.vertexNum = i.vertexCount;
    geomObject.triangles.indexBuffer = indexData->buf;
    geomObject.triangles.vertexFormat = nri::Format::RGB32_SFLOAT;
    geomObject.triangles.indexOffset = i.indexOffset * sizeof(uint32_t);
    geomObject.triangles.indexNum = i.indexCount;
    geomObject.triangles.indexType = nri::IndexType::UINT32;
    geomObject.triangles.vertexStride = sizeof(Vertex);

    nri::AccelerationStructureDesc blasDesc{};
    blasDesc.flags = nri::AccelerationStructureBuildBits::PREFER_FAST_TRACE;
    blasDesc.type = nri::AccelerationStructureType::BOTTOM_LEVEL;
    blasDesc.geometryObjects = &geomObject;
    blasDesc.instanceOrGeometryObjectNum = 1;
    auto        blas = m_interface.allocateAccStructure(blasDesc);
    std::string blasName = "blas" + std::to_string(meshIdx);
    m_interface.SetAccelerationStructureDebugName(*(blas->acc), blasName.c_str());
    m_blas.push_back(blas);
    geomObjects.push_back(geomObject);
    scratchBufferSize =
        std::max(scratchBufferSize,
                 m_interface.GetAccelerationStructureBuildScratchBufferSize(*(blas->acc)));
    ++meshIdx;
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

  for (auto& mesh : sceneGraph->m_meshMap) {
    nri::GeometryObjectInstance& instance = geometryInstances[mesh.second];
    instance.accelerationStructureHandle =
        m_interface.GetAccelerationStructureHandle(*(m_blas[mesh.second]->acc));
    instance.instanceId = mesh.second;
    glm::mat4 globalTrans = sceneGraph->getGlobalTransformsFromIdx(mesh.first);
    toTransformMatrixKHR(globalTrans, instance.transform);
    instance.mask = 0xff;
  }
  MtxBufferAllocInfo allocInfo{};
  allocInfo._memLocation = nri::MemoryLocation::HOST_UPLOAD;
  //if debugging on nsight
  // allocInfo._desc = {.size = std::max((size_t) 4096, helper::GetByteSizeOf(geometryInstances)),
  //                    .usageMask = nri::BufferUsageBits::ACCELERATION_STRUCTURE_BUILD_READ};
  allocInfo._desc = {.size = helper::GetByteSizeOf(geometryInstances),
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
      *cmd, sceneGraph->m_meshMap.size(), instanceBuffer->getBuf(), 0,
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
  const uint64_t         tableAlignment = deviceDesc.rayTracingShaderTableAlignment;

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
  CameraDesc MainCameraDesc;
  MainCameraDesc.nearZ = 0.1f;
  MainCameraDesc.aspectRatio = float(GetWindowResolution().x) / float(GetWindowResolution().y);
  GetCameraDescFromInputDevices(MainCameraDesc);
  // m_cameras.front().Update(mainCameraDesc, m_frameIndex);
  m_cameras.front().updateState(MainCameraDesc, m_frameIndex, m_Timer.GetFrameTime());
  CameraUniform uniform;
  uniform.ViewToWorld = m_cameras.front().state.mViewToWorld;
  uniform.ViewToClip = m_cameras.front().state.mViewToClip;
  uniform.ClipToView = m_cameras.front().state.mClipToView;
  uniform.ClipToWorld = m_cameras.front().state.mClipToWorld;
  uniform.WorldToClip = m_cameras.front().state.mWorldToClip;
  uniform.WorldToView = m_cameras.front().state.mWorldToView;
  uniform.camPosFov.x = m_cameras.front().state.position.x;
  uniform.camPosFov.y = m_cameras.front().state.position.y;
  uniform.camPosFov.z = m_cameras.front().state.position.z;
  uniform.camPosFov.w = 5.0f;

  void* data =
      m_interface.MapBuffer(*(m_cameras.front().camUniformBuffer->buf), 0, sizeof(CameraUniform));
  memcpy(data, &uniform, sizeof(uniform));
  m_interface.UnmapBuffer(*(m_cameras.front().camUniformBuffer->buf));
  data = nullptr;
  // MTX_INFO("camera pos is x:{},y:{},z:{}", m_cameras.front().state.position.x,
  //          m_cameras.front().state.position.y, m_cameras.front().state.position.z);
}

void MTXRenderer::PrepareFrame(uint32_t frameIndex) {
  updateCamera(m_Timer.GetFrameTime());
}

void MTXRenderer::RenderFrame(uint32_t frameIndex) {
  const uint32_t       bufferedFrameIndex = frameIndex % BUFFERED_FRAME_MAX_NUM;
  const FrameResource& frame = m_frameResource[bufferedFrameIndex];

  if (frameIndex >= BUFFERED_FRAME_MAX_NUM) {
    m_interface.Wait(*frame._fence, 1 + frameIndex - BUFFERED_FRAME_MAX_NUM);
    for (auto& cmdPool : frame._commandPools) { m_interface.ResetCommandAllocator(*cmdPool); }
  }

  const uint32_t swapImageIndex = m_interface.AcquireNextSwapChainTexture(*m_swapChain);

  nri::TextureBarrierDesc textureTransitions[2] = {};
  nri::BarrierGroupDesc   barrierGroupDesc = {};

  //record
  nri::CommandBuffer& cmdBuf = *frame._commandBuffers[0];
  m_interface.BeginCommandBuffer(cmdBuf, m_descriptorPool);
  {
    textureTransitions[0].texture = (frame._frameTexture.tex);
    textureTransitions[0].after = {nri::AccessBits::COLOR_ATTACHMENT,
                                   nri::Layout::COLOR_ATTACHMENT};
    textureTransitions[0].mipNum = 1;
    textureTransitions[0].arraySize = 1;

    textureTransitions[1].texture = m_rayTracingTexture->tex;
    textureTransitions[1].before = {
        frameIndex == 0 ? nri::AccessBits::UNKNOWN : nri::AccessBits::COPY_SOURCE,
        frameIndex == 0 ? nri::Layout::UNKNOWN : nri::Layout::COPY_SOURCE};
    textureTransitions[1].after = {nri::AccessBits::SHADER_RESOURCE_STORAGE,
                                   nri::Layout::SHADER_RESOURCE_STORAGE};
    textureTransitions[1].mipNum = 1;
    textureTransitions[1].arraySize = 1;

    barrierGroupDesc.textureNum = 2;
    barrierGroupDesc.textures = textureTransitions;
    m_interface.CmdBarrier(cmdBuf, barrierGroupDesc);

    m_interface.CmdSetPipelineLayout(cmdBuf, *(m_rayTracingPipeline->pipelineLayout));
    m_interface.CmdSetPipeline(cmdBuf, m_rayTracingPipeline->getPipeline());
    for (uint32_t i = 0; i < helper::GetCountOf(m_descriptorSets); ++i) {
      m_interface.CmdSetDescriptorSet(cmdBuf, i, *m_descriptorSets[i], nullptr);
    }

    nri::DispatchRaysDesc desc = {};
    desc.raygenShader = {m_shaderBindingTable->buf, 0, m_shaderGroupIdentifierSize,
                         m_shaderGroupIdentifierSize};
    desc.missShaders = {m_shaderBindingTable->buf, m_missShaderOffset, m_shaderGroupIdentifierSize,
                        m_shaderGroupIdentifierSize};
    desc.hitShaderGroups = {m_shaderBindingTable->buf, m_hitShaderGroupOffset,
                            m_shaderGroupIdentifierSize, m_shaderGroupIdentifierSize};
    desc.x = (uint16_t) GetWindowResolution().x;
    desc.y = (uint16_t) GetWindowResolution().y;
    desc.z = 1;
    m_constant.accumFrameCount = m_cameras.front().isDirty ? 0 : m_constant.accumFrameCount;
    m_constant.maxBounce = m_maxBounce;
    m_interface.CmdSetConstants(cmdBuf, 0, &m_constant, sizeof(MtxRayTracingPushConstant));
    if (m_constant.accumFrameCount < m_constant.maxSampleCount) {
      m_interface.CmdDispatchRays(cmdBuf, desc);
      m_constant.accumFrameCount++;
    }
/*-------------------ray tracing end,post process begin---------------------------------*/

    textureTransitions[1].before = textureTransitions[1].after;
    textureTransitions[1].after = {nri::AccessBits::SHADER_RESOURCE, nri::Layout::SHADER_RESOURCE};
    barrierGroupDesc.textures = &textureTransitions[1];
    barrierGroupDesc.textureNum = 1;
    m_interface.CmdBarrier(cmdBuf, barrierGroupDesc);

    // m_interface.CmdCopyTexture(cmdBuf, *(frame._frameTexture.tex), nullptr,
    //                            *(m_rayTracingTexture->tex), nullptr);

    nri::AttachmentsDesc attachmentsDesc = {};
    attachmentsDesc.colorNum = 1;
    attachmentsDesc.colors = &frame._frameTexture.imageView;
    m_interface.CmdBeginRendering(cmdBuf,attachmentsDesc);
    {
      {
        helper::Annotation annotation(m_interface,cmdBuf,"postProcess");
        const nri::Viewport viewport = {0.0,0.0,(float)m_WindowResolution.x,(float)m_WindowResolution.y};
        m_interface.CmdSetViewports(cmdBuf,&viewport,1);
        m_interface.CmdSetPipelineLayout(cmdBuf,m_postProcessPipeline->getPipelineLayout());
        m_interface.CmdSetPipeline(cmdBuf,m_postProcessPipeline->getPipeline());
        m_interface.CmdSetConstants(cmdBuf,0,&m_postConstant,sizeof(m_postConstant));
        m_interface.CmdSetDescriptorSet(cmdBuf,0,*m_postDescriptorSets[0],nullptr);
        nri::Rect scissor = {0,0,(nri::Dim_t)m_WindowResolution.x,(nri::Dim_t)m_WindowResolution.y};
        m_interface.CmdSetScissors(cmdBuf,&scissor,1);
        m_interface.CmdDraw(cmdBuf,{3,1,0,0});
      }
    }
    m_interface.CmdEndRendering(cmdBuf);

    textureTransitions[0].before = textureTransitions[0].after;
    textureTransitions[0].after = {nri::AccessBits::UNKNOWN, nri::Layout::PRESENT};

    barrierGroupDesc.textureNum = 1;
    barrierGroupDesc.textures = textureTransitions;
    m_interface.CmdBarrier(cmdBuf, barrierGroupDesc);
  }
  m_interface.EndCommandBuffer(cmdBuf);

  {
    nri::FenceSubmitDesc signalFence = {};
    signalFence.fence = frame._fence;
    signalFence.value = 1 + frameIndex;

    nri::QueueSubmitDesc submitDesc = {};
    submitDesc.commandBufferNum = 1;
    submitDesc.commandBuffers = &frame._commandBuffers[0];
    submitDesc.signalFenceNum = 1;
    submitDesc.signalFences = &signalFence;
    m_interface.QueueSubmit(m_interface.getGraphicQueue(), submitDesc);
  }

  m_interface.QueuePresent(*m_swapChain);
}
}// namespace MTX