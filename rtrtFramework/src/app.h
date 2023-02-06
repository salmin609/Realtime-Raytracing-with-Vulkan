
#include "camera.h"


class App
{
public:
    GLFWwindow* GLFW_window;
    App(int argc, char** argv);
    bool doApiDump;
    
    Camera myCamera;
    bool m_show_gui = true;
    void updateCamera(VkApp* app);
};
