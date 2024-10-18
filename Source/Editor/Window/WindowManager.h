#pragma once
#include "Window.h"

#include <osg/ref_ptr>

#include <vector>
#include <type_traits>
#include <iostream>
#include <functional>

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

        void draw()
        {
            for (auto windowIt = mWindows.begin(); windowIt != mWindows.end();)
            {
                Window* window = windowIt->get();
                if (!window->draw() && !window->isSingleton())
                    windowIt = mWindows.erase(windowIt);
                else
                    ++windowIt;
            }
        }

        void foreachWindow(const std::function<void(Window*)>& func)
        {
            for (Window* window : mWindows)
                func(window);
        }

    protected:
        std::vector<osg::ref_ptr<Window>> mWindows;
    };
}
