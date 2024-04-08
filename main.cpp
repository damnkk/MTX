#include <iostream>
#include "NRIFramework.h"

class myApp:public SampleBase{
    bool Initialize(nri::GraphicsAPI graphicsAPI) override{return false;};
    void PrepareFrame(uint32_t frameIndex) override{};
    void RenderFrame(uint32_t frameIndex) override{};
    
    std::string m_appName= "fuck";
};


int main(){
    myApp* app = new myApp;
    std::cout<<"tset"<<std::endl;
}
