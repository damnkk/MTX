#ifndef __DENOISER_H__
#define __DENOISER_H__
#include<NRIFramework.h>
#include<NRD.h>
#include <memory>
#include "NRDIntegration.h"

namespace MTX{
class MTXRenderer;
class MTXTexture;

class MTXDenoiser{
public:
    MTXDenoiser(MTXRenderer* renderer);
    void init();
    void destroy();
    enum DenoiseRT{
        MV,
        VIEWZ,
        _COUNT
    };

private:
    MTXRenderer* m_renderer;
    std::unique_ptr<nrd::Integration> m_integration;
    std::vector<std::shared_ptr<MTXTexture>> m_userPool;
};

}// namespace MTX



#endif // __DENOISER_H__