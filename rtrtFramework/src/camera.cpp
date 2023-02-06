
#include <iostream>
#include "camera.h"

Camera::Camera() : spin(-20.0), tilt(10.66), ry(0.57), front(0.1), back(1000.0),
                   eye({2.279976, 1.677772, 6.640697})
{
}

glm::mat4 Camera::perspective(const float aspect)
{
    glm::mat4 P = glm::mat4(1.f);

    float rx = ry*aspect;
    P[0][0] = 1.0f/rx;
    P[1][1] = -1.0f/ry; // Because Vulkan does y upside-down.
    P[2][2] = -back/(back-front);  // Becasue Vulkan wants [front,back] mapped to [0,1]
    P[3][2] = -(front*back)/(back-front);
    P[2][3] = -1;
    P[3][3] = 0;
    return P;

}

glm::mat4 Camera::view()
{
    glm::mat4 SPIN = glm::rotate(spin*3.14159f/180.0f, glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 TILT = glm::rotate(tilt*3.14159f/180.0f, glm::vec3(1.0, 0.0, 0.0));
    glm::mat4 TRAN = glm::translate(-eye);
    return TILT*SPIN*TRAN;
}

void Camera::mouseMove(const float x, const float y)
{
    float dx = x-posx;
    float dy = y-posy;
    spin += dx/3;
    tilt += dy/3;
    posx = x;
    posy = y;
}

void Camera::setMousePosition(const float x, const float y)
{
    posx = x;
    posy = y;
}

void Camera::wheel(const int dir)
{
    printf("wheel: %d\n", dir);
}
