#include "WindowWithAsyncInput.h"
#include <iostream>
#include <condition_variable>
#include <cmath>

static WindowWithAsyncInput* windowWithAsyncInput_static_ptr = nullptr;

WindowWithAsyncInput::WindowWithAsyncInput(const std::string&             in_title,
                                           unsigned int                   in_width,
                                           unsigned int                   in_height)
{
    if (not hasGlfwInit)
        InitGlfw();

    assert(windowWithAsyncInput_static_ptr == nullptr);
    windowWithAsyncInput_static_ptr = this;

    bool windows_create_var = false;
    std::condition_variable cv;

    inputThread_uptr = std::make_unique<std::thread>(
        [&, this]() {
            {
                std::lock_guard<std::mutex> thread_lk(controlMutex);

                glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
                window = glfwCreateWindow(in_width, in_height, in_title.c_str(), nullptr, nullptr);

#ifdef NDEBUG // -------
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
#endif        // -------

                glfwSetKeyCallback(window, key_glfw_callback);
                glfwSetMouseButtonCallback(window, mouse_button_glfw_callback);
                glfwSetCursorPosCallback(window, mouse_cursor_glfw_callback);

                if (glfwRawMouseMotionSupported())
                    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

                glfwGetCursorPos(window, &lastMousePosition.first, &lastMousePosition.second);
                moduloOfMousePosition = std::make_pair(0.f, 0.f);

                windows_create_var = true;
            }
            cv.notify_one();

            bool breakLoop = false;
            while(not breakLoop)
            {
                glfwWaitEvents();
                {
                    std::lock_guard guard(controlMutex);
                    breakLoop = closeInputThread;
                }
            }
        });

    std::unique_lock wait_lk(controlMutex);
    cv.wait(wait_lk, [&windows_create_var] {return windows_create_var; });
}

WindowWithAsyncInput::~WindowWithAsyncInput()
{
    {
        std::lock_guard guard(controlMutex);
        closeInputThread = true;
    }

    glfwPostEmptyEvent();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    inputThread_uptr->join();
    inputThread_uptr.reset();

    if (hasGlfwInit)
        glfwTerminate();

    hasGlfwInit = false;
}

std::vector<std::string> WindowWithAsyncInput::GetRequiredInstanceExtensions()
{
    if (not hasGlfwInit)
        InitGlfw();

    uint32_t count;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);

    std::vector<std::string> return_vector;
    for(size_t i=0; i != count; ++i) {
        const char* this_extension_cstr = extensions[i];
        return_vector.emplace_back(this_extension_cstr);
    }

    return return_vector;
}

void WindowWithAsyncInput::InitGlfw()
{
    assert(hasGlfwInit == false);

    glfwInit();
    glfwSetErrorCallback([]( int error_code, const char * str )
                         { std::cerr << "GLFW: " << error_code << " : " << str;});

    hasGlfwInit = true;
}

bool WindowWithAsyncInput::ShouldClose() const
{
    return glfwWindowShouldClose(window);
}

void WindowWithAsyncInput::CallbackKeyPressRelease(int key, int action)
{
    std::lock_guard guard(controlMutex);
    if (action == GLFW_PRESS) {
        for( const auto& this_lambda : callbackListOnKeyPress)   this_lambda(key);
    } else if (action == GLFW_RELEASE) {
        for( const auto& this_lambda : callbackListOnKeyRelease) this_lambda(key);
    }
}

void WindowWithAsyncInput::CallbackMouseMovement(double xpos, double ypos)
{
    double dx, dy;
    dx = xpos - lastMousePosition.first;
    dy = ypos - lastMousePosition.second;

    double summed_dx, summed_dy;
    summed_dx = dx + moduloOfMousePosition.first;
    summed_dy = dy + moduloOfMousePosition.second;

    double summed_dx_floored, summed_dy_floored;
    summed_dx_floored = (summed_dx >= 0.0) ? std::floor(summed_dx) : std::ceil(summed_dx);
    summed_dy_floored = (summed_dy >= 0.0) ? std::floor(summed_dy) : std::ceil(summed_dy);

    long int_dx, int_dy;
    int_dx = long(summed_dx_floored);
    int_dy = long(summed_dy_floored);

    {
        std::lock_guard guard(controlMutex);
        for (const auto &this_lambda: callbackListOnMouseMove) {
            this_lambda(int_dx, int_dy);
        }
    }

    lastMousePosition.first = xpos;
    lastMousePosition.second = ypos;

    moduloOfMousePosition.first = summed_dx - summed_dx_floored;
    moduloOfMousePosition.second = summed_dy - summed_dy_floored;

    assert(std::abs(moduloOfMousePosition.first) < 1.f && std::abs(moduloOfMousePosition.second) < 1.f);
}

void WindowWithAsyncInput::AddCallbackKeyPressLambda(std::function<void(int)> lambda)
{
    std::lock_guard guard(controlMutex);
    callbackListOnKeyPress.emplace_back(lambda);
}

void WindowWithAsyncInput::AddCallbackKeyReleaseLambda(std::function<void(int)> lambda)
{
    std::lock_guard guard(controlMutex);
    callbackListOnKeyRelease.emplace_back(lambda);
}

void WindowWithAsyncInput::AddCallbackMouseMoveLambda(std::function<void(long, long)> lambda)
{
    std::lock_guard guard(controlMutex);
    callbackListOnMouseMove.emplace_back(lambda);
}

void WindowWithAsyncInput::DeleteCallbacks()
{
    std::lock_guard guard(controlMutex);
    callbackListOnKeyPress.clear();
    callbackListOnKeyRelease.clear();
    callbackListOnMouseMove.clear();
}

void key_glfw_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    assert(windowWithAsyncInput_static_ptr != nullptr);
    windowWithAsyncInput_static_ptr->CallbackKeyPressRelease(key, action);
}

void mouse_button_glfw_callback(GLFWwindow* window, int button, int action, int mods)
{
    assert(windowWithAsyncInput_static_ptr != nullptr);
    windowWithAsyncInput_static_ptr->CallbackKeyPressRelease(button, action);
}

void mouse_cursor_glfw_callback(GLFWwindow* window, double xpos, double ypos)
{
    assert(windowWithAsyncInput_static_ptr != nullptr);
    windowWithAsyncInput_static_ptr->CallbackMouseMovement(xpos, ypos);

}
