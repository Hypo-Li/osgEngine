#pragma once
#include "Window.h"

#include <vector>
#include <type_traits>
#include <iostream>

namespace xxx::editor
{
    class WindowManager
    {
    public:
        static WindowManager& get()
        {
            static WindowManager windowManager;
            return windowManager;
        }

        template <typename T, typename... Args>
        std::enable_if_t<std::is_base_of_v<Window, T>, T*> createWindow(Args... args)
        {
            T* window = new T(args...);
            mWindows.push_back(window);
            return window;
        }

        bool hasWindowWantCaptureEvents() const
        {
            bool result = false;
            for (Window* window : mWindows)
                result |= window->isWantCaptureEvents();
            return result;
        }

        void draw() const
        {
            for (Window* window : mWindows)
                window->draw();
        }

    protected:
        std::vector<Window*> mWindows;
    };
}
