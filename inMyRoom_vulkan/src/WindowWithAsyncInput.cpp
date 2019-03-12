#include "WindowWithAsyncInput.h"


WindowWithAsyncInput::WindowWithAsyncInput(const Anvil::WindowPlatform	  platform,
                                           const std::string&             in_title,
                                           unsigned int                   in_width,
                                           unsigned int                   in_height,
                                           bool                           in_closable)
    :should_thread_close(false)
{
    std::promise<Anvil::WindowUniquePtr> prom_m_window_ptr;
    std::future<Anvil::WindowUniquePtr> fut_m_window_ptr = prom_m_window_ptr.get_future();

    std::promise<THREADID_T> prom_threadID;
    std::future<THREADID_T> fut_threadID = prom_threadID.get_future();

#ifdef _WIN32

    window_thread_ptr = std::make_unique<std::thread>([=, &prom_m_window_ptr, &prom_threadID]()
    {
        /* Create a window */

        Anvil::WindowUniquePtr m_window_ptr = Anvil::WindowFactory::create_window(platform,
                                                                                  in_title,
                                                                                  in_width,
                                                                                  in_height,
                                                                                  in_closable, /* in_closable */
                                                                                  nullptr);

        prom_m_window_ptr.set_value(std::move(m_window_ptr));
        prom_threadID.set_value(::GetCurrentThreadId());

        ::ShowCursor(FALSE);

        RAWINPUTDEVICE Rid[1];
        Rid[0].usUsagePage = 0x01;
        Rid[0].usUsage = 0x02;
        Rid[0].dwFlags = RIDEV_NOLEGACY;	// adds HID mouse and also ignores legacy mouse messages
        Rid[0].hwndTarget = 0;

        RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));

        while (true)
        {

            MSG msg;


            ::GetMessage(&msg,
                         0,
                         0,
                         0);

            if (should_thread_close)
                break;

            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }

    });

#else

    window_thread_ptr = std::make_unique<std::thread>([=, &prom_m_window_ptr, &prom_threadID]()
    {
        /* Create a window */

        Anvil::WindowUniquePtr m_window_ptr = Anvil::WindowFactory::create_window(platform,
                                                                                  in_title,
                                                                                  in_width,
                                                                                  in_height,
                                                                                  in_closable, /* in_closable */
                                                                                  nullptr);

        Anvil::Window* window_ptr = m_window_ptr.get();

        prom_m_window_ptr.set_value(std::move(m_window_ptr));
        prom_threadID.set_value(pthread_self());

        while(true)
        {
            xcb_generic_event_t* event_ptr = static_cast<XCBLoader*>(window_ptr->get_XCBLoader())->get_procs_table()->pfn_xcbWaitForEvent(static_cast<xcb_connection_t*>(window_ptr->get_connection()));

            if (should_thread_close)
                break;

            window_ptr->msg_callback(event_ptr);
        }
    });

#endif

    m_window_ptr = fut_m_window_ptr.get();
    threadID = fut_threadID.get();
}

WindowWithAsyncInput::~WindowWithAsyncInput()
{
    should_thread_close = true;

#ifdef _WIN32

    ::PostThreadMessage(
        threadID,
        WM_NULL,
        0,
        0
    );
#else
    xcb_configure_notify_event_t event;
    event.response_type = XCB_DESTROY_NOTIFY;

    static_cast<XCBLoader*>(m_window_ptr->get_XCBLoader())->get_procs_table()->pfn_xcbSendEvent(static_cast<xcb_connection_t*>(m_window_ptr->get_connection()),
                                                                                                true,
                                                                                                m_window_ptr->m_window,
                                                                                                XCB_EVENT_MASK_STRUCTURE_NOTIFY,
                                                                                                (char*)&event);


    static_cast<XCBLoader*>(m_window_ptr->get_XCBLoader())->get_procs_table()->pfn_xcbFlush(static_cast<xcb_connection_t*>(m_window_ptr->get_connection()));
#endif

    std::this_thread::sleep_for(std::chrono::microseconds(10));

    window_thread_ptr->join();
    window_thread_ptr.reset();

    m_window_ptr.reset();
}
