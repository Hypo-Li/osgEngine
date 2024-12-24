#include "Window.h"

namespace xxx::editor
{
    class Hierarchy : public Window
    {
    public:
        Hierarchy() : Window("Hierarchy")
        {

        }

        void drawTree(Entity* entity)
        {
            Context& context = Context::get();
            const char* name = entity->getName().c_str();
            size_t childrenCount = entity->getChildrenCount();
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;
            if (context.getActivedEntity() == entity)
                flags |= ImGuiTreeNodeFlags_Selected;
            if (!childrenCount)
                flags |= ImGuiTreeNodeFlags_Leaf;
            
            if (ImGui::TreeNodeEx(name, flags))
            {
                if (ImGui::IsItemClicked())
                {
                    context.setActivedEntity(entity);
                }
                for (size_t i = 0; i < childrenCount; ++i)
                {
                    drawTree(entity->getChild(i));
                }
                ImGui::TreePop();
            }
        }

        virtual bool draw() override
        {
            if (!mVisibility)
                return true;

            if (ImGui::Begin(mTitle.c_str(), &mVisibility))
            {
                Scene* scene = Context::get().getScene();
                if (scene)
                {
                    Entity* rootEntity = scene->getRootEntity();
                    size_t childrenCount = rootEntity->getChildrenCount();
                    for (size_t i = 0; i < childrenCount; ++i)
                    {
                        drawTree(rootEntity->getChild(i));
                    }
                }
            }
            ImGui::End();

            return true;
        }
    };
}
