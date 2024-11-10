#include<renderer.h>
#include<windowManager/winManager.h>
const uint32_t MAX_SUB_WINDOW = 256;
namespace MTX{

void WindowManager::init(MTXRenderer* app){
    _app = app;
    _subWindows.resize(MAX_SUB_WINDOW,nullptr);
    _subWindowState.resize(MAX_SUB_WINDOW, WindowState::INVALID);
    _windowUsage.resize(MAX_SUB_WINDOW);
    for(int i = 0;i<_windowUsage.size();++i){
        _windowUsage[i] = i;
    }
    //ManBar
    std::shared_ptr<UIWindow> mainBar = std::make_shared<UIWindow>(
        [](UIWindow* thisWindow,std::vector<void*> params){
            ImGui::BeginMainMenuBar();
            if(ImGui::BeginMenu("File")){
                if(ImGui::MenuItem("loadHDR")){
                   
                }
                if(ImGui::MenuItem("loadScene")){
           
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    );
    _mainWindow.push_back(mainBar);
    //MainWindow1
    std::shared_ptr<UIWindow> mainWindow1 = std::make_shared<UIWindow>(
    [](UIWindow* thisWindow,std::vector<void*> params){
        ImGui::Begin("MainWindow1");
        if(ImGui::Button("SubWindow1")){
            int subWindowID = thisWindow->_subWindow["SubWindow1"];
            WindowManager::WindowState newState =WindowManager::WindowState( thisWindow->getWindowState(thisWindow->_subWindow["SubWindow1"])^1);
            thisWindow->setWindowState(subWindowID,newState);
        }
        ImGui::End();
        for(auto& [first,second] : thisWindow->_subWindow){
            auto subWindow = thisWindow->_manager->_subWindows[second];
            if(subWindow&&thisWindow->_manager->_subWindowState[second]==WindowManager::WindowState::SHOWN){

                subWindow->_onGUI(subWindow.get(),{});
            }else if(!subWindow){
                std::cerr<<"You may not finished the subWindow definition"<<std::endl;
            }
        }
    });
    // mainWindow1->_onGUI = lambda1;
    // MainWindow1->SubWindow1
    std::function<void(UIWindow*,std::vector<void*>)> lambdaMain1Sub1 = [](UIWindow* thisWindow,std::vector<void*> params){
        ImGui::Begin("SubWindow1");
        ImGui::Text("This is SubWindow1");
        ImGui::End();
    };
    mainWindow1->_manager = this;
    mainWindow1->addSubWindow("SubWindow1",[](UIWindow* thisWindow,std::vector<void*> params){
        ImGui::Begin("SubWindow1");
        ImGui::Text("This is SubWindow1");
        ImGui::End();
    });
    _mainWindow.push_back(mainWindow1);
}

int WindowManager::allocWindow(std::function<void(UIWindow*,std::vector<void*>)> renderFunc){
    int res = _windowsCounter++;
    if(res >= MAX_SUB_WINDOW){
        return -1;
    }
    _subWindows[res] = std::make_shared<WindowManager::UIWindow>(renderFunc);
    _subWindows[res]->_manager = this;
    _subWindowState[res] = WindowState::HIDDEN;
    return res;
}

void WindowManager::onGUI(){
   for(auto& window:_mainWindow){
    window->_onGUI(window.get(),{});
   }
}

WindowManager::WindowState WindowManager::UIWindow::getWindowState(int id){
    return _manager->_subWindowState[id];
}

void WindowManager::destroyWindow(int id){
    assert(id>=0 && id<_subWindowState.size());
    _subWindowState[id] = WindowState::INVALID;
    _subWindows[id] = nullptr;
    _windowUsage[--_windowsCounter] = id;
}

void WindowManager::UIWindow::addSubWindow(std::string name, std::function<void(UIWindow*,std::vector<void*>)> renderFunc){
    if(_subWindow.find(name)!=_subWindow.end()){
        std::cerr<<"Can not create a sub window with the same name: "<<name<<"!"<<std::endl;
        return;
    }
    int res = _manager->allocWindow(renderFunc);
    if(res>=0){
        _subWindow[name]=res;
    }
}

void WindowManager::UIWindow::setWindowState(int id, WindowState state){
    this->_manager->_subWindowState[id] = state;
}

} // namespace MTX