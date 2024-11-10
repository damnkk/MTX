#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H
#include "NRIFramework.h"
#include<functional>
namespace MTX{
class MTXRenderer;
class WindowManager{
public:
    void init(MTXRenderer* renderer);
    enum WindowState{
        INVALID = -1,
        HIDDEN  = 0,
        SHOWN   = 1
    };

    struct UIWindow{
        UIWindow() = default;
        UIWindow(std::function<void(UIWindow*,std::vector<void*>)> renderFunc):_onGUI(renderFunc){}
        void addSubWindow(std::string name, std::function<void(UIWindow*,std::vector<void*>)> renderFunc);
        WindowState getWindowState(int id);
        void setWindowState(int id, WindowState state);
        std::function<void(UIWindow*,std::vector<void*>)> _onGUI;
        WindowManager* _manager;
        std::unordered_map<std::string,int> _subWindow;
    };
    void onGUI();
private:
    int allocWindow(std::function<void(UIWindow*,std::vector<void*>)> renderFunc);
    void destroyWindow(int id);
    friend struct UIWindow;
    std::vector<std::shared_ptr<UIWindow>> _subWindows;
    std::vector<WindowState> _subWindowState;
    std::vector<std::shared_ptr<UIWindow>> _mainWindow;
    std::vector<int> _windowUsage;
    std::atomic_int _windowsCounter=0;
    MTXRenderer* _app;
};

}//namespace MTX



#endif // WINDOW_MANAGER_H