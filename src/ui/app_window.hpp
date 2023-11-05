#pragma once

#include <memory>
#include <utility>

#include <daxa/daxa.hpp>
#include <GLFW/glfw3.h>

#include "rml/system_glfw.hpp"

struct AppWindow {
    std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> glfw_window{nullptr, &glfwDestroyWindow};
    daxa::Swapchain swapchain{};
    daxa_i32vec2 size{};

    int glfw_active_modifiers{};
    Rml::Context *rml_context{};

    std::function<void()> on_resize{};
    std::function<void()> on_close{};
    using RmlKeyDownCallback = std::function<bool(Rml::Context *context, Rml::Input::KeyIdentifier key, int key_modifier, float native_dp_ratio, bool priority)>;
    RmlKeyDownCallback key_down_callback{};

    AppWindow() = default;
    explicit AppWindow(daxa::Device device, daxa_i32vec2 size);

    void update();
};
