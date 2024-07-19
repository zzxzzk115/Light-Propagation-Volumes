#pragma once

#include "vgfw.hpp"

struct Camera
{
    struct CameraUniform
    {
        alignas(16) glm::vec3 position;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 projection;
        alignas(16) glm::mat4 inverseView;
        alignas(16) glm::mat4 inverseProjection;
    } data;

    float fov   = 60.0f;
    float zNear = 0.1f;
    float zFar  = 100.0f;

    float yaw = 0, pitch = 0;
    float speed = 10, sensitivity = 10;
    bool  isCaptureCursor = false;

    void updateData(const std::shared_ptr<vgfw::window::Window>& window);
    void update(const std::shared_ptr<vgfw::window::Window>& window, float dt);
};