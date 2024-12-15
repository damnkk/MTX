#include <renderer.h>
#include <denoiser.h>
#include <NRDIntegration.hpp>
#include <sceneLoader.h>
namespace MTX {
constexpr uint32_t BUFFER_FRAME_MAX_NUM = 3;
constexpr bool NRD_ALLOW_DESCRIPTOR_CACHING         = true;
constexpr bool NRD_PROMOTE_FLOAT16_TO_32            = false;
constexpr bool NRD_DEMOTE_FLOAT32_TO_16             = false;

MTXDenoiser::MTXDenoiser(MTXRenderer* renderer) : m_renderer(renderer){
    m_integration = std::make_unique<nrd::Integration>();
}
void MTX::MTXDenoiser::init(){
    const nrd::DenoiserDesc denoiserDesc[]={
        {static_cast<nrd::Identifier>(nrd::Denoiser::REBLUR_DIFFUSE_SPECULAR),nrd::Denoiser::REBLUR_DIFFUSE_SPECULAR},
        {static_cast<nrd::Identifier>(nrd::Denoiser::SIGMA_SHADOW_TRANSLUCENCY),nrd::Denoiser::SIGMA_SHADOW_TRANSLUCENCY}
    };
    nrd::InstanceCreationDesc instanceCreationDesc = {};
    instanceCreationDesc.denoisersNum =::helper::GetCountOf(denoiserDesc);
    instanceCreationDesc.denoisers = denoiserDesc;

    nrd::IntegrationCreationDesc desc = {};
    desc.name = "MTX Denoiser";
    desc.bufferedFramesNum = BUFFER_FRAME_MAX_NUM;
    desc.enableDescriptorCaching = NRD_ALLOW_DESCRIPTOR_CACHING;
    desc.demoteFloat32to16 = NRD_DEMOTE_FLOAT32_TO_16;
    desc.promoteFloat16to32 = NRD_PROMOTE_FLOAT16_TO_32;
    desc.resourceWidth = m_renderer->GetWindowResolution().x;
    desc.resourceHeight = m_renderer->GetWindowResolution().y;
    const auto interface =  m_renderer->getInterface();
    m_integration->Initialize(desc, instanceCreationDesc, interface->getDevice(), *interface, *interface);
    m_integration->CreatePipelines();
    
    createTexture();
    createPipeline();
    createDescriptors();
}

void MTXDenoiser::destroy(){
    m_integration->Destroy();
}

MtxTextureAllocInfo   getTextureAllocInfo(std::string name,nri::TextureUsageBits usage,
                                                                nri::Format format,uint2 texSize,nri::Sample_t sampleNum = 1) {
  MtxTextureAllocInfo info = {};
  info._name = std::move(name);
  info._desc = {
   .type = nri::TextureType::TEXTURE_2D,
  .usage = usage,
  .format = format,
  .width = static_cast<nri::Dim_t>(texSize.x),
  .height = static_cast<nri::Dim_t>(texSize.y),
  .depth = 1,
  .mipNum = 1,
  .layerNum = 1,
  .sampleNum = sampleNum};
return info;
}

void MTXDenoiser::createTexture() {
    m_userTexturePool.resize(DenoiseRT::RT_COUNT);
    MtxTextureAllocInfo info = {};
    info = getTextureAllocInfo("Texture::ViewZ",nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::R32_SFLOAT
      ,m_renderer->GetWindowResolution());
    m_userTexturePool[VIEWZ] =m_renderer->getInterface()->allocateTexture(info);
    info = getTextureAllocInfo("Texture::MV",nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::RGBA16_SFLOAT
      ,m_renderer->GetWindowResolution());
    m_userTexturePool[MV] = m_renderer->getInterface()->allocateTexture(info);
    info = getTextureAllocInfo("Texture::Normal_Roughness",nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::R10_G10_B10_A2_UNORM
      ,m_renderer->GetWindowResolution());
    m_userTexturePool[Normal_Roughness] = m_renderer->getInterface()->allocateTexture(info);
    info = getTextureAllocInfo("Texture::PsrThroughput",nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::R10_G10_B10_A2_UNORM
      ,m_renderer->GetWindowResolution());
    m_userTexturePool[PsrThroughput] = m_renderer->getInterface()->allocateTexture(info);
    info = getTextureAllocInfo("Texture::BaseColor_Metalness",nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::RGBA8_SRGB,
      m_renderer->GetWindowResolution());
    m_userTexturePool[BaseColor_Metalness] = m_renderer->getInterface()->allocateTexture(info);
    info = getTextureAllocInfo("Texture::DirectLighting", nri::TextureUsageBits::SHADER_RESOURCE | nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,
                               nri::Format::R11_G11_B10_UFLOAT, m_renderer->GetWindowResolution());
    m_userTexturePool[DirectLighting] = m_renderer->getInterface()->allocateTexture(info);
    info = getTextureAllocInfo("Texture::DirectEmission", nri::TextureUsageBits::SHADER_RESOURCE | nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,
                               nri::Format::R11_G11_B10_UFLOAT, m_renderer->GetWindowResolution());
    m_userTexturePool[DirectEmission] = m_renderer->getInterface()->allocateTexture(info);
    info = getTextureAllocInfo("Texture::Shadow",nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::RGBA8_UNORM,
      m_renderer->GetWindowResolution());
    m_userTexturePool[Shadow] = m_renderer->getInterface()->allocateTexture(info);
    info = getTextureAllocInfo("Texture::Diff",nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::RGBA16_SFLOAT,
      m_renderer->GetWindowResolution());
    m_userTexturePool[Diff] = m_renderer->getInterface()->allocateTexture(info);
    info = getTextureAllocInfo("Texture::Spec",nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::RGBA16_SFLOAT,
      m_renderer->GetWindowResolution());
    m_userTexturePool[Spec] = m_renderer->getInterface()->allocateTexture(info);
    info = getTextureAllocInfo("Texture:Unfiltered_Prenumbra",nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::R16_SFLOAT,
      m_renderer->GetWindowResolution());
    m_userTexturePool[Unfiltered_Penumbra] = m_renderer->getInterface()->allocateTexture(info);
    info = getTextureAllocInfo("Texture::Unfiltered_Diff",nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::RGBA16_SFLOAT,
      m_renderer->GetWindowResolution());
    m_userTexturePool[Unfiltered_Diff] = m_renderer->getInterface()->allocateTexture(info);
    info = getTextureAllocInfo(
        "Texture::Unfiltered_Spec",
        nri::TextureUsageBits::SHADER_RESOURCE | nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,
        nri::Format::RGBA16_SFLOAT, m_renderer->GetWindowResolution());
    m_userTexturePool[Unfiltered_Spec] = m_renderer->getInterface()->allocateTexture(info);

    info = getTextureAllocInfo("Texture::Unfiltered_Translucency", nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::RGBA8_UNORM,m_renderer->GetWindowResolution());
    m_userTexturePool[Unfiltered_Translucency] = m_renderer->getInterface()->allocateTexture(info);
    info = getTextureAllocInfo("Texture::Composed",nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::RGBA16_SFLOAT,
      m_renderer->GetWindowResolution());
    m_userTexturePool[Composed] = m_renderer->getInterface()->allocateTexture(info);
    info = getTextureAllocInfo("Texture::ComposedDiff",nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::RGBA16_SFLOAT,
      m_renderer->GetWindowResolution());
    m_userTexturePool[ComposedDiff] = m_renderer->getInterface()->allocateTexture(info);
    info = getTextureAllocInfo("Texture::ComposedSpec_ViewZ",nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::RGBA16_SFLOAT,
      m_renderer->GetWindowResolution());
    m_userTexturePool[ComposedSpec_ViewZ] = m_renderer->getInterface()->allocateTexture(info);
    nri::Descriptor* descriptor = nullptr;
    for (auto& texture : m_userTexturePool) {
      nri::Texture2DViewDesc viewDesc = {.texture = texture->tex,
                                          .viewType = nri::Texture2DViewType::SHADER_RESOURCE_2D,
                                          .format = texture->desc.format,
                                          .mipOffset = 0,
                                          .mipNum = texture->desc.mipNum,
                                          .layerOffset = 0,
                                          .layerNum = texture->desc.layerNum};
      MTX_CHECK(m_renderer->getInterface()->CreateTexture2DView(viewDesc, descriptor));
      m_descriptors.push_back(descriptor);
      if (texture->desc.usage & nri::TextureUsageBits::SHADER_RESOURCE_STORAGE) {
        viewDesc.viewType = nri::Texture2DViewType::SHADER_RESOURCE_STORAGE_2D;
        MTX_CHECK(m_renderer->getInterface()->CreateTexture2DView(viewDesc, descriptor));
        m_descriptors.push_back(descriptor);
      }
    }
}

void MTXDenoiser::createPipeline() {
  m_pipelines.resize(DenoisePipeline::PIPLINE_COUNT);
  std::vector<nri::DescriptorRangeDesc> rangeDesc1 = {
    //set0 ---> rayTracing texture/ tlas / camera uniform
    {
        0, 1, nri::DescriptorType::STORAGE_TEXTURE, nri::StageBits::RAYGEN_SHADER,
        nri::DescriptorRangeBits::PARTIALLY_BOUND},
      {1, 1, nri::DescriptorType::ACCELERATION_STRUCTURE, nri::StageBits::RAYGEN_SHADER|nri::StageBits::CLOSEST_HIT_SHADER},
      {2, 1, nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::RAY_TRACING_SHADERS}
  };
  std::vector<nri::DescriptorRangeDesc> rangeDesc2 = {
      //set1 ---> material uniform/ vertices/ indices/ instance info/textureSampler/EnvAccelBuffer
      {0, 1, nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::RAY_TRACING_SHADERS },
      {1, 1, nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::RAY_TRACING_SHADERS},
      {2, 1, nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::RAY_TRACING_SHADERS},
      {3, 1, nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::RAY_TRACING_SHADERS},
      {4, 1, nri::DescriptorType::SAMPLER, nri::StageBits::RAY_TRACING_SHADERS},
      {5, 1, nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::RAY_TRACING_SHADERS,nri::DescriptorRangeBits::PARTIALLY_BOUND}
      };
  
      std::vector<nri::DescriptorRangeDesc> rangeDesc3 = {{
          //set2 ---> scene textures
          0, static_cast<uint32_t>(m_renderer->m_sceneLoader->getSceneTextures().size()),nri::DescriptorType::TEXTURE, nri::StageBits::RAY_TRACING_SHADERS,
              nri::DescriptorRangeBits::VARIABLE_SIZED_ARRAY}};
      std::vector<nri::DescriptorRangeDesc> rangeDesc4={
            //set3 ---> env textures
      {0, static_cast<uint32_t>(m_renderer->m_sceneLoader->getEnvTextures().size()),nri::DescriptorType::TEXTURE, nri::StageBits::RAY_TRACING_SHADERS, 
      nri::DescriptorRangeBits::VARIABLE_SIZED_ARRAY}
      };

      std::vector<nri::DescriptorRangeDesc> rangeDesc5 = {
          //set4 ---> primitives info
          {0, static_cast<uint32_t>(m_renderer->m_sceneLoader->getMeshes().size()), nri::DescriptorType::STRUCTURED_BUFFER,
           nri::StageBits::RAY_TRACING_SHADERS, nri::DescriptorRangeBits::VARIABLE_SIZED_ARRAY},
      };

      std::vector<nri::DescriptorRangeDesc> rangeDesc6 = {
          //shader read only and shader storage image
          {0, 31, nri::DescriptorType::TEXTURE, nri::StageBits::RAY_TRACING_SHADERS, nri::DescriptorRangeBits::PARTIALLY_BOUND},
          {0, 31, nri::DescriptorType::STORAGE_TEXTURE, nri::StageBits::RAY_TRACING_SHADERS, nri::DescriptorRangeBits::PARTIALLY_BOUND}};

      nri::RootConstantDesc rootCostantDesc = {0, sizeof(MTXRenderer::MtxRayTracingPushConstant), nri::StageBits::RAY_TRACING_SHADERS};

      nri::DescriptorSetDesc descriptorSetDescs[] = {
          {0, rangeDesc1.data(), ::helper::GetCountOf(rangeDesc1), nullptr, 0}, {1, rangeDesc2.data(), ::helper::GetCountOf(rangeDesc2), nullptr, 0},
          {2, rangeDesc3.data(), ::helper::GetCountOf(rangeDesc3), nullptr, 0}, {3, rangeDesc4.data(), ::helper::GetCountOf(rangeDesc4), nullptr, 0},
          {4, rangeDesc5.data(), ::helper::GetCountOf(rangeDesc5), nullptr, 0}, {5, rangeDesc6.data(), ::helper::GetCountOf(rangeDesc6), nullptr, 0}};

  nri:nri::PipelineLayoutDesc layoutDesc {};
  layoutDesc.descriptorSets = descriptorSetDescs;
  layoutDesc.descriptorSetNum = ::helper::GetCountOf(descriptorSetDescs);
  layoutDesc.shaderStages = nri::StageBits::RAY_TRACING_SHADERS;
  layoutDesc.rootConstants = &rootCostantDesc;
  layoutDesc.rootConstantNum = 1;
  MTX_CHECK(m_renderer->getInterface()->CreatePipelineLayout(
      m_renderer->getInterface()->getDevice(), layoutDesc, m_rtLayout));

  auto deviceDesc =
      m_renderer->getInterface()->GetDeviceDesc(m_renderer->getInterface()->getDevice());
  ::utils::ShaderCodeStorage shaderCodeStorage;

  //create trace opaque pipeline
  ShaderLoader shaderLoader(m_renderer->getInterface());
  shaderLoader .addShader("traceOpaque.rgen", "raygen", shaderCodeStorage)
      .addShader("traceOpaque.rmiss", "miss", shaderCodeStorage)
      .addShader("traceOpaque.rchit", "closest_hit", shaderCodeStorage);
  nri::ShaderLibrary shaderLib = {};
  shaderLib.shaders = shaderLoader.getShaderDesc().data();
  shaderLib.shaderNum = shaderLoader.getShaderDesc().size();
  std::vector<nri::ShaderGroupDesc> shaderGroups;
  for (uint32_t i = 0; i < shaderLoader.getShaderDesc().size(); ++i) {
    shaderGroups.push_back({i+1});
  }
  nri::RayTracingPipelineDesc pipelineDesc = {};
  pipelineDesc.pipelineLayout = m_rtLayout;
  pipelineDesc.recursionDepthMax = 5;
  pipelineDesc.payloadAttributeSizeMax = 128;
  pipelineDesc.intersectionAttributeSizeMax = 128;
  pipelineDesc.shaderGroupDescNum = shaderGroups.size();
  pipelineDesc.shaderGroupDescs = shaderGroups.data();
  pipelineDesc.shaderLibrary = &shaderLib;
  MtxPipelineAllocateInfo traceOpaquePipelineAllocInfo = {.pipelineDesc = &pipelineDesc,.pipelineType = PipelineType::RayTracing,.name = "TraceOpaqueRTPipeline"};
  m_pipelines[DenoisePipeline::TraceOpaque] = m_renderer->getInterface()->allocatePipeline(traceOpaquePipelineAllocInfo);

  const uint64_t identifierSize = deviceDesc.rayTracingShaderGroupIdentifierSize;
  const uint64_t tableAlignment = deviceDesc.shaderBindingTableAlignment;
  m_shaderGroupIdentifierSize = identifierSize;
  m_missShaderOffset = ::helper::Align(identifierSize, tableAlignment);
  m_hitShaderOffset =
      ::helper::Align(m_missShaderOffset + identifierSize * shaderLoader.getShaderTypeNum()[int(nri::StageBits::MISS_SHADER) >> 12], tableAlignment);
  const uint64_t SBTSize = ::helper::Align(
      m_hitShaderOffset + identifierSize * shaderLoader.getShaderTypeNum()[int(nri::StageBits::CLOSEST_HIT_SHADER) >> 12], tableAlignment);
  MtxBufferAllocInfo bufferInfo{};
  bufferInfo._name = "Denoiser_SBTBuffer";
  bufferInfo._desc.size = SBTSize;
  bufferInfo._desc.usage = nri::BufferUsageBits::SHADER_BINDING_TABLE;
  bufferInfo._memLocation = nri::MemoryLocation::DEVICE;
  m_shaderBindingTable = m_renderer->m_interface.allocateBuffer(bufferInfo);
  std::vector<uint8_t> tableData((size_t) SBTSize, 0);
  int                  groupIndex = 0;
  for (int i = 0; i < shaderLoader.getShaderTypeNum()[int(nri::StageBits::RAYGEN_SHADER) >> 12]; ++i) {
    m_renderer->getInterface()->WriteShaderGroupIdentifiers(m_pipelines[DenoisePipeline::TraceOpaque]->getPipeline(), groupIndex, 1,
                                                            tableData.data() + i * identifierSize);
    groupIndex++;
  }
  for (int i = 0; i < shaderLoader.getShaderTypeNum()[int(nri::StageBits::MISS_SHADER) >> 12]; ++i) {
    m_renderer->getInterface()->WriteShaderGroupIdentifiers(m_pipelines[DenoisePipeline::TraceOpaque]->getPipeline(), groupIndex, 1,
                                                            tableData.data() + m_missShaderOffset + i * identifierSize);
    groupIndex++;
  }
  for (int i = 0; i < shaderLoader.getShaderTypeNum()[int(nri::StageBits::CLOSEST_HIT_SHADER) >> 12]; ++i) {
    m_renderer->getInterface()->WriteShaderGroupIdentifiers(m_pipelines[DenoisePipeline::TraceOpaque]->getPipeline(), groupIndex, 1,
                                                            tableData.data() + m_hitShaderOffset + i * identifierSize);
    groupIndex++;
  }
  nri::BufferUploadDesc dataDesc = {};
  dataDesc.buffer = m_shaderBindingTable->buf;
  dataDesc.data = tableData.data();
  dataDesc.dataSize = sizeof(uint8_t) * tableData.size();
  dataDesc.bufferOffset = 0;
  dataDesc.after = {nri::AccessBits::UNKNOWN};
  MTX_CHECK(m_renderer->getInterface()->UploadData(m_renderer->getInterface()->getTransferQueue(),nullptr,0,&dataDesc,1));

  //create composition pipeline

  nri::DescriptorRangeDesc compranges0[4] = {
    //camera uniform ,sampler, denoise shader resource, denoise shader storage resource
    {0, 1, nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::COMPUTE_SHADER,
     nri::DescriptorRangeBits::PARTIALLY_BOUND},
    {1, 1, nri::DescriptorType::SAMPLER, nri::StageBits::COMPUTE_SHADER,
     nri::DescriptorRangeBits::PARTIALLY_BOUND},
    {2, 31, nri::DescriptorType::TEXTURE, nri::StageBits::COMPUTE_SHADER,
     nri::DescriptorRangeBits::PARTIALLY_BOUND},
    {3, 31, nri::DescriptorType::STORAGE_TEXTURE, nri::StageBits::COMPUTE_SHADER,
     nri::DescriptorRangeBits::PARTIALLY_BOUND}
  };

  nri::RootConstantDesc comprootCostantDesc = {0,sizeof(MTXRenderer::MtxRayTracingPushConstant),nri::StageBits::COMPUTE_SHADER};

  nri::DescriptorSetDesc compdescriptorSetDescs [] ={
    {0,compranges0,::helper::GetCountOf(compranges0),nullptr,0}
  };
  layoutDesc.descriptorSets = compdescriptorSetDescs;
  layoutDesc.descriptorSetNum = ::helper::GetCountOf(compdescriptorSetDescs);
  layoutDesc.rootConstants = &comprootCostantDesc;
  layoutDesc.rootConstantNum = 1;
  layoutDesc.shaderStages = nri::StageBits::COMPUTE_SHADER;
  MTX_CHECK(m_renderer->getInterface()->CreatePipelineLayout(m_renderer->getInterface()->getDevice(),layoutDesc,m_csLayout))
  nri::ComputePipelineDesc compositePipelineDesc = {};
  compositePipelineDesc.pipelineLayout = m_csLayout;
  compositePipelineDesc.shader = ::utils::LoadShader(deviceDesc.graphicsAPI, "composition.cs", shaderCodeStorage);
  MtxPipelineAllocateInfo compositePipelineAllocInfo = {};
  compositePipelineAllocInfo.name = "CompositionPipeline";
  compositePipelineAllocInfo.pipelineDesc = &compositePipelineDesc;
  compositePipelineAllocInfo.pipelineType = PipelineType::Compute;
  m_pipelines[DenoisePipeline::Composition] = m_renderer->getInterface()->allocatePipeline(compositePipelineAllocInfo);
}

void MTXDenoiser::createDescriptors() {
  nri::DescriptorSet* descriptorSet = nullptr;
  m_descriptorsets.resize(DescType_COUNT);
  //rt pipeline set allocation
  m_renderer->getInterface()->AllocateDescriptorSets(*(m_renderer->m_descriptorPool), *m_rtLayout,
                                                     Accel_Desc, &m_descriptorsets[Accel_Desc], 1, 0);
  m_renderer->getInterface()->AllocateDescriptorSets(*(m_renderer->m_descriptorPool), *m_rtLayout,
                                                     TraceOpaque_Common_Desc, &m_descriptorsets[TraceOpaque_Common_Desc],
                                                     1, 0);
  m_renderer->getInterface()->AllocateDescriptorSets(*(m_renderer->m_descriptorPool), *m_rtLayout,TraceOpaque_SceneTex_Desc,
                                                   &m_descriptorsets[TraceOpaque_SceneTex_Desc],
                                                      1, m_renderer->m_sceneLoader->getSceneTextures().size());
  m_renderer->getInterface()->AllocateDescriptorSets(*(m_renderer->m_descriptorPool), *m_rtLayout, TraceOpaque_EnvTex_Desc,
                                                      &m_descriptorsets[TraceOpaque_EnvTex_Desc], 1,
                                                      m_renderer->m_sceneLoader->getEnvTextures().size());
  m_renderer->getInterface()->AllocateDescriptorSets(*(m_renderer->m_descriptorPool), *m_rtLayout, TraceOpaque_PrimitiveInfo_Desc,
                                                     &m_descriptorsets[TraceOpaque_PrimitiveInfo_Desc], 1,
                                                     m_renderer->m_sceneLoader->getMeshes().size());
  m_renderer->getInterface()->AllocateDescriptorSets(*(m_renderer->m_descriptorPool), *m_rtLayout, TraceOpaque_DenoiseRT_Desc,
                                                     &m_descriptorsets[TraceOpaque_DenoiseRT_Desc], 1, 0);
  
  //cs pipeline set allocation
  m_renderer->getInterface()->AllocateDescriptorSets(*(m_renderer->m_descriptorPool), *m_csLayout, 0, &m_descriptorsets[Composition_Desc], 1, 0);

  //trace opaque set
  {
    const nri::Descriptor* resources[] = {
      m_descriptors[RT_DESC_COMPOSED_DIFF],
      m_descriptors[RT_DESC_COMPOSED_SPEC_VIEWZ]
    };

    const nri::Descriptor* storageResources[] = {m_descriptors[RT_DESC_MV_STORAGE],
                                                 m_descriptors[RT_DESC_VIEWZ_STORAGE],
                                                 m_descriptors[RT_DESC_NORMAL_ROUGHNESS_STORAGE],
                                                 m_descriptors[RT_DESC_BASE_COLOR_METALNESS_STORAGE],
                                                 m_descriptors[RT_DESC_DIRECT_LIGHTING_STORAGE],
                                                 m_descriptors[RT_DESC_DIRECT_EMISSION_STORAGE],
                                                 m_descriptors[RT_DESC_PSR_THROUGHPUT_STORAGE],
                                                 m_descriptors[RT_DESC_UNFILTERED_PENUMBRA_STORAGE],
                                                 m_descriptors[RT_DESC_UNFILTERED_TRANSLUCENCY_STORAGE],
                                                 m_descriptors[RT_DESC_DIFFUSE_STORAGE],
                                                 m_descriptors[RT_DESC_SPECULAR_STORAGE]};
    const nri::DescriptorRangeUpdateDesc updateDesc[2] = {
      {resources, ::helper::GetCountOf(resources)},
        {storageResources,::helper::GetCountOf(storageResources)}
      };
    m_renderer->getInterface()->UpdateDescriptorRanges(*(m_descriptorsets[TraceOpaque_DenoiseRT_Desc]),0,helper::GetCountOf(updateDesc),updateDesc);
  }
  {
    //composition set
    const nri::Descriptor* resources[] = {
        m_descriptors[RT_DESC_VIEWZ],           m_descriptors[RT_DESC_NORMAL_ROUGHNESS], m_descriptors[RT_DESC_BASE_COLOR_METALNESS],
        m_descriptors[RT_DESC_DIRECT_LIGHTING], m_descriptors[RT_DESC_DIRECT_EMISSION],  m_descriptors[RT_DESC_PSR_THROUGHPUT],
        m_descriptors[RT_DESC_SHADOW],          m_descriptors[RT_DESC_DIFFUSE],          m_descriptors[RT_DESC_SPECULAR]};

    const nri::Descriptor* storageResources[] = {m_descriptors[RT_DESC_COMPOSED_DIFF_STORAGE], m_descriptors[RT_DESC_COMPOSED_SPEC_VIEWZ_STORAGE]};
    const nri::DescriptorRangeUpdateDesc updateDesc[2] = {{resources, ::helper::GetCountOf(resources)},
                                                          {storageResources, ::helper::GetCountOf(storageResources)}};
    m_renderer->getInterface()->UpdateDescriptorRanges(*(m_descriptorsets[Composition_Desc]),2,::helper::GetCountOf(updateDesc),updateDesc);
  }
}

}// namespace MTX
