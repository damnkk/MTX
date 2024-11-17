#ifndef __DENOISER_H__
#define __DENOISER_H__
#include<NRIFramework.h>
#include<NRD.h>
#include <memory>
#include "NRDIntegration.h"

namespace MTX{
class MTXRenderer;
class MtxTexture;
class  MtxPipeline;
enum DenoiseRT {
  VIEWZ,
  MV,
  Normal_Roughness,
  PsrThroughput,
  BaseColor_Metalness,
  DirectLighting,
  DirectEmission,
  Shadow,
  Diff,
  Spec,
  Unfiltered_Penumbra,
  Unfiltered_Diff,
  Unfiltered_Spec,
  Unfiltered_Translucency,
  Validation,
  Composed,
  DlssOutput,
  PreFinal,
  Final,
  // History
 ComposedDiff,
 ComposedSpec_ViewZ,
 TaaHistory,
 TaaHistoryPrev,
  RT_COUNT
};

enum DenoiserRTDescriptor {
    RT_DESC_VIEWZ,
    RT_DESC_VIEWZ_STORAGE,
    RT_DESC_MV,
    RT_DESC_MV_STORAGE,
    RT_DESC_NORMAL_ROUGHNESS,
    RT_DESC_NORMAL_ROUGHNESS_STORAGE,
    RT_DESC_PSR_THROUGHPUT,
    RT_DESC_PSR_THROUGHPUT_STORAGE,
    RT_DESC_BASE_COLOR_METALNESS,
    RT_DESC_BASE_COLOR_METALNESS_STORAGE,
    RT_DESC_DIRECT_LIGHTING,
    RT_DESC_DIRECT_LIGHTING_STORAGE,
    RT_DESC_DIRECT_EMISSION,
    RT_DESC_DIRECT_EMISSION_STORAGE,
    RT_DESC_SHADOW,
    RT_DESC_SHADOW_STORAGE,
    RT_DESC_DIFFUSE,
    RT_DESC_DIFFUSE_STORAGE,
    RT_DESC_SPECULAR,
    RT_DESC_SPECULAR_STORAGE,
    RT_DESC_UNFILTERED_PENUMBRA,
    RT_DESC_UNFILTERED_PENUMBRA_STORAGE,
    RT_DESC_UNFILTERED_DIFFUSE,
    RT_DESC_UNFILTERED_DIFFUSE_STORAGE,
    RT_DESC_UNFILTERED_SPECULAR,
    RT_DESC_UNFILTERED_SPECULAR_STORAGE,
    RT_DESC_UNFILTERED_TRANSLUCENCY,
    RT_DESC_UNFILTERED_TRANSLUCENCY_STORAGE,
    RT_DESC_VALIDATION,
    RT_DESC_COMPOSED,
    RT_DESC_COMPOSED_DIFF,
    RT_DESC_COMPOSED_DIFF_STORAGE,
    RT_DESC_COMPOSED_SPEC_VIEWZ,
    RT_DESC_COMPOSED_SPEC_VIEWZ_STORAGE,
    RT_DESC_TAA_HISTORY,
    RT_DESC_TAA_HISTORY_STORAGE,
    RT_DESC_TAA_HISTORY_PREV,
    RT_DESC_TAA_HISTORY_PREV_STORAGE,
    RT_DESC_DLSS_OUTPUT,
    RT_DESC_DLSS_OUTPUT_STORAGE,
    RT_DESC_PRE_FINAL,
    RT_DESC_PRE_FINAL_STORAGE,
    RT_DESC_FINAL,
    RT_DESC_FINAL_STORAGE,
    RT_DESC_COMPOSED_DIFF_PREV,
    RT_DESC_COMPOSED_DIFF_PREV_STORAGE,
    RT_DESC_COMPOSED_SPEC_VIEWZ_PREV,
    RT_DESC_COMPOSED_SPEC_VIEWZ_PREV_STORAGE,
    RT_DESC_COUNT
};

enum DenoisePipeline {
       TraceOpaque,
       Composition,
       TraceTransparent,
       PIPLINE_COUNT
};

enum DescriptorSetType {
    Common,

};


class MTXDenoiser{
public:
    MTXDenoiser(MTXRenderer* renderer);
    void init();
    void destroy();
    void createTexture();
    void createPipeline();
    void  createDescritptors();



   private:
    nrd::ReblurSettings                      m_reblurSettings;
    nrd::SigmaSettings                       m_sigmaSettings;
    nrd::RelaxSettings                       m_relaxSettings;
    MTXRenderer*                             m_renderer;
    std::unique_ptr<nrd::Integration>        m_integration;
    std::vector<std::shared_ptr<MtxTexture>> m_userTexturePool;
    std::vector<std::shared_ptr<MtxPipeline>> m_pipelines;
    nri::PipelineLayout*                       m_pipelineLayout;
};

}// namespace MTX



#endif // __DENOISER_H__