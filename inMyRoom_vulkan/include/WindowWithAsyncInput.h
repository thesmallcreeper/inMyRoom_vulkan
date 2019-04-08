#pragma once

#include <memory>
#include <functional>
#include <thread>
#include <future>

#include "misc/window_factory.h"

#ifdef _WIN32
    typedef DWORD THREADID_T;
#else
    typedef pthread_t THREADID_T;
#endif

class WindowWithAsyncInput;

typedef std::unique_ptr<WindowWithAsyncInput, std::function<void(WindowWithAsyncInput*)> > WindowWithAsyncInputUniquePtr;

class WindowWithAsyncInput
{
public:
    WindowWithAsyncInput(const Anvil::WindowPlatform	platform,
                         const std::string&             in_title,
                         unsigned int                   in_width,
                         unsigned int                   in_height,
                         bool                           in_closable);
    ~WindowWithAsyncInput();

    Anvil::Window* GetWindowPtr() const
    {
        return m_window_ptr.get();
    }

private:

    Anvil::WindowUniquePtr m_window_ptr;
    std::unique_ptr<std::thread> window_thread_ptr;
    THREADID_T threadID;

    volatile bool should_thread_close;
};

