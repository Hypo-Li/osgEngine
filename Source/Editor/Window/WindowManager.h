#pragma once
#include "Window.h"

#include <osg/ref_ptr>

#include <vector>
#include <unordered_set>
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
            osg::ref_ptr<T> window = new T(args...);
            if (mWindowTitleSet.find(window->getTitle()) == mWindowTitleSet.end())
            {
                mWindows.push_back(window);
                mWindowTitleSet.insert(window->getTitle());
                return window;
            }
            return nullptr;
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
        std::list<osg::ref_ptr<Window>> mWindows;
        std::unordered_set<std::string_view> mWindowTitleSet;
    };
}
