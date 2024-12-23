#include "Window.h"

namespace xxx::editor
{
    class Hierarchy : public Window
    {
    public:
        Hierarchy() : Window("Hierarchy")
        {

        }

        virtual bool draw() override
        {
            if (!mVisibility)
                return true;

            if (ImGui::Begin(mTitle.c_str(), &mVisibility))
            {

            }
            ImGui::End();

            return true;
        }
    };
}
