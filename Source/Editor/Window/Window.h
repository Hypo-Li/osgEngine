#pragma once
#include "ThirdParty/imgui/imgui.h"

#include <osg/Referenced>
#include <string>

namespace xxx::editor
{
    class Window : public osg::Referenced
    {
    public:
        Window(const std::string& title) :
            mTitle(title),
            mVisibility(true)
        {

        }

        virtual bool draw() = 0;

        virtual bool isSingleton() const
        {
            return true;
        }

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

        void setVisibility(bool visibility)
        {
            mVisibility = visibility;
        }

        bool getVisibility() const
        {
            return mVisibility;
        }

    protected:
        std::string mTitle;
        bool mVisibility;
    };
}
