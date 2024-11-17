#include <renderer.h>
#include <denoiser.h>
#include <NRDIntegration.hpp>
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
    m_userTexturePool.resize(DenoiseRT::RT_COUNT);
    createTexture();
    createPipeline();
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
      info = getTextureAllocInfo("Texture::DirectLighting",nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::RGBA16_SFLOAT,
        m_renderer->GetWindowResolution());
      m_userTexturePool[DirectLighting] = m_renderer->getInterface()->allocateTexture(info);
      info = getTextureAllocInfo("Texture::DirectEmission",nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::RGBA16_SFLOAT,
        m_renderer->GetWindowResolution());
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
      info = getTextureAllocInfo("Texture::Unfiltered_Spec",nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::RGBA16_SFLOAT,
        m_renderer->GetWindowResolution());
      m_userTexturePool[Spec] = m_renderer->getInterface()->allocateTexture(info);
      info = getTextureAllocInfo("Texture::Composed",nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::RGBA16_SFLOAT,
        m_renderer->GetWindowResolution());
      m_userTexturePool[Composed] = m_renderer->getInterface()->allocateTexture(info);
      info = getTextureAllocInfo("Texture::ComposedDiff",nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::RGBA16_SFLOAT,
        m_renderer->GetWindowResolution());
      m_userTexturePool[ComposedDiff] = m_renderer->getInterface()->allocateTexture(info);
      info = getTextureAllocInfo("Texture::ComposedSpec_ViewZ",nri::TextureUsageBits::SHADER_RESOURCE|nri::TextureUsageBits::SHADER_RESOURCE_STORAGE,nri::Format::RGBA16_SFLOAT,
        m_renderer->GetWindowResolution());
      m_userTexturePool[ComposedSpec_ViewZ] = m_renderer->getInterface()->allocateTexture(info);
}

void MTXDenoiser::createPipeline() {
  m_pipelines.resize(DenoisePipeline::PIPLINE_COUNT);
  nri::DescriptorRangeDesc ranges0 ={0,1,nri::DescriptorType::ACCELERATION_STRUCTURE,nri::StageBits::RAY_TRACING_SHADERS};
  nri::DescriptorRangeDesc ranges1[6] = {
    //set1 ---> material uniform/ vertices/ indices/ instance info/CameraUniform/textureSampler
    {0,1,nri::DescriptorType::STRUCTURED_BUFFER,nri::StageBits::RAY_TRACING_SHADERS|nri::StageBits::COMPUTE_SHADER,nri::DescriptorRangeBits::PARTIALLY_BOUND},
    {1, 1, nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::CLOSEST_HIT_SHADER|nri::StageBits::COMPUTE_SHADER,nri::DescriptorRangeBits::PARTIALLY_BOUND},
    {2, 1, nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::CLOSEST_HIT_SHADER|nri::StageBits::COMPUTE_SHADER,nri::DescriptorRangeBits::PARTIALLY_BOUND},
    {3, 1, nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::CLOSEST_HIT_SHADER|nri::StageBits::COMPUTE_SHADER,nri::DescriptorRangeBits::PARTIALLY_BOUND},
    {4, 1, nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::CLOSEST_HIT_SHADER|nri::StageBits::COMPUTE_SHADER,nri::DescriptorRangeBits::PARTIALLY_BOUND},
    {5, 1, nri::DescriptorType::SAMPLER, nri::StageBits::RAY_TRACING_SHADERS|nri::StageBits::COMPUTE_SHADER,nri::DescriptorRangeBits::PARTIALLY_BOUND}
  };

  nri::DescriptorRangeDesc ranges2[2] = {
//shader read only and shader storage image
    {0,31,nri::DescriptorType::TEXTURE,nri::StageBits::RAY_TRACING_SHADERS|nri::StageBits::COMPUTE_SHADER,nri::DescriptorRangeBits::PARTIALLY_BOUND},
    {0,31,nri::DescriptorType::STORAGE_TEXTURE,nri::StageBits::RAY_TRACING_SHADERS|nri::StageBits::COMPUTE_SHADER,nri::DescriptorRangeBits::PARTIALLY_BOUND}
  };

  nri::RootConstantDesc rootCostantDesc = {0,sizeof(MTXRenderer::MtxRayTracingPushConstant),nri::StageBits::RAY_TRACING_SHADERS|nri::StageBits::COMPUTE_SHADER};

  nri::DescriptorSetDesc descriptorSetDescs [] ={
    {0,&ranges0,1,nullptr,0},
    {1,ranges1,::helper::GetCountOf(ranges1),nullptr,0},
    {2,ranges2,::helper::GetCountOf(ranges2),nullptr,0}
};


  nri:nri::PipelineLayoutDesc layoutDesc {};
  layoutDesc.descriptorSets = descriptorSetDescs;
  layoutDesc.descriptorSetNum = ::helper::GetCountOf(descriptorSetDescs);
  layoutDesc.shaderStages = nri::StageBits::COMPUTE_SHADER|nri::StageBits::RAY_TRACING_SHADERS;
  layoutDesc.rootConstants = &rootCostantDesc;
  layoutDesc.rootConstantNum = 1;
  MTX_CHECK(m_renderer->getInterface()->CreatePipelineLayout(m_renderer->getInterface()->getDevice(),layoutDesc,m_pipelineLayout));

   //create trace opaque pipeline
  nri::RayTracingPipelineDesc pipelineDesc = {};


  //create composition pipeline
  nri::ComputePipelineDesc compositePipelineDesc = {};
  compositePipelineDesc.pipelineLayout = m_pipelineLayout;
  // compositePipelineDesc.shader =
  MtxPipelineAllocateInfo compositePipelineAllocInfo = {};
  compositePipelineAllocInfo.name = "CompositionPipeline";
  compositePipelineAllocInfo.pipelineDesc = &compositePipelineDesc;
  compositePipelineAllocInfo.pipelineType = PipelineType::Compute;
  // m_pipelines[DenoisePipeline::Composition] = m_renderer->getInterface()->allocatePipeline(compositePipelineAllocInfo);




}


void MTXDenoiser::createDescritptors() {

}


}// namespace MTX
