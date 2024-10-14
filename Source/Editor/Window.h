#pragma once
#include "ThirdParty/imgui/imgui.h"

namespace xxx::editor
{
    class Window
    {
    public:
        virtual void draw() = 0;

        virtual bool isWantCaptureEvents() const
        {
            return false;
        }

    protected:

    };
}
