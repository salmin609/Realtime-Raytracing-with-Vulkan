#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

struct InputState {bool lmb=false; bool mmb=false; bool rmb=false; 
    bool shift=false; bool ctrl=false; bool alt=false;};

class Camera
{
 public:
    float ry;
    float front;
    float back;

    float spin;
    float tilt;

    glm::vec3 eye;

    bool lmb=false;
    bool mmb=false;
    bool rmb=false; 
    bool shift=false;
    bool ctrl=false;
    bool alt=false;

    float posx = 0.0;
    float posy = 0.0;
    
    Camera();
    glm::mat4 perspective(const float aspect);
    glm::mat4 view();

    void mouseMove(const float x, const float y);
    void setMousePosition(const float x, const float y);
    void wheel(const int dir);
};
