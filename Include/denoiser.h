#ifndef __DENOISER_H__
#define __DENOISER_H__
#include<NRIFramework.h>
#include<NRD.h>
#include<NRDIntegration.hpp>
#include <memory>
namespace MTX{
class MTXRenderer;
class MTXTexture;

class MTXDenoiser{
public:
    MTXDenoiser(MTXRenderer* renderer):m_renderer(renderer){};
    void init();
    enum DenoiseRT{
        MV,
        VIEWZ,
        _COUNT
    };

private:
    MTXRenderer* m_renderer;
    nrd::Integration m_integration;
    std::vector<std::shared_ptr<MTXTexture>> m_userPool;
};

}// namespace MTX



#endif // __DENOISER_H__