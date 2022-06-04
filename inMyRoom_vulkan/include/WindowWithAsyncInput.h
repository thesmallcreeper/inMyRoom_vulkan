#pragma once

#include <vector>
#include <string>
#include <functional>
#include <mutex>
#include <thread>

#include "vulkan/vulkan.hpp"
#include "GLFW/glfw3.h"

class WindowWithAsyncInput
{
public:
    WindowWithAsyncInput(const std::string&             title,
                         unsigned int                   width,
                         unsigned int                   height,
                         bool                           fullscreen);
    ~WindowWithAsyncInput();

    void CallbackKeyPressRelease(int key, int action);
    void CallbackMouseMovement(double xpos, double ypos);

    void AddCallbackKeyPressLambda(std::function<void(int)> lambda);
    void AddCallbackKeyReleaseLambda(std::function<void(int)> lambda);
    void AddCallbackMouseMoveLambda(std::function<void(long,long)> lambda);
    void DeleteCallbacks();

    bool ShouldClose() const;

    GLFWwindow* GetGlfwWindow() const {return window;}
    static std::vector<std::string> GetRequiredInstanceExtensions();

private:
    std::vector<std::function<void(int)>> callbackListOnKeyPress;
    std::vector<std::function<void(int)>> callbackListOnKeyRelease;
    std::vector<std::function<void(long,long)>> callbackListOnMouseMove;

    std::pair<double, double> lastMousePosition;
    std::pair<double, double> moduloOfMousePosition;

    std::unique_ptr<std::thread> inputThread_uptr;
    std::mutex controlMutex;
    bool closeInputThread = false;

    GLFWwindow* window = nullptr;

    static void InitGlfw();
    inline static bool hasGlfwInit = false;
};

void key_glfw_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_glfw_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_cursor_glfw_callback(GLFWwindow* window, double xpos, double ypos);

