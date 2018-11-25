#pragma once

#include <memory>
#include <functional>

#include "misc/window_factory.h"

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

	Anvil::WindowUniquePtr m_window_ptr;

private:

	std::unique_ptr<std::thread> window_thread_ptr;
	DWORD threadID;

	volatile bool should_thread_close;
};

