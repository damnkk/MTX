#include<renderer.h>
#include<denoiser.h>
#include<NRDIntegration.hpp>
namespace MTX{
const uint32_t BUFFER_FRAME_MAX_NUM = 3;
constexpr bool NRD_ALLOW_DESCRIPTOR_CACHING         = true;
constexpr bool NRD_PROMOTE_FLOAT16_TO_32            = false;
constexpr bool NRD_DEMOTE_FLOAT32_TO_16             = false;

MTXDenoiser::MTXDenoiser(MTXRenderer* renderer) : m_renderer(renderer){
    m_integration = std::make_unique<nrd::Integration>();
}
void MTX::MTXDenoiser::init(){
    const nrd::DenoiserDesc denoiserDesc[]={
        {nrd::Identifier(nrd::Denoiser::REBLUR_DIFFUSE_SPECULAR),nrd::Denoiser::REBLUR_DIFFUSE_SPECULAR},
        {nrd::Identifier(nrd::Denoiser::SIGMA_SHADOW_TRANSLUCENCY),nrd::Denoiser::SIGMA_SHADOW_TRANSLUCENCY}
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
    auto interface =  m_renderer->getInterface();
    m_integration->Initialize(desc, instanceCreationDesc, interface->getDevice(), *interface, *interface);
    m_integration->CreatePipelines();
}

void MTXDenoiser::destroy(){
    m_integration->Destroy();
}

}// namespace MTX
