#pragma once
#include "ThirdParty/imgui/imgui.h"

#include <string>

namespace xxx::editor
{
    class Window
    {
    public:
        Window(const std::string& title) :
            mTitle(title),
            mIsShow(true)
        {

        }

        virtual void draw() = 0;

        virtual bool isWantCaptureEvents() const
        {
            return false;
        }

        const std::string& getTitle() const
        {
            return mTitle;
        }

        void setTitle(const std::string& title)
        {
            mTitle = title;
        }

        bool isShow() const
        {
            return mIsShow;
        }

    protected:
        std::string mTitle;
        bool mIsShow;
    };
}
