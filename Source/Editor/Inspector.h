#pragma once
#include "Window.h"
#include "Widget.h"
#include <Engine/Core/Entity.h>
#include <Engine/Core/Context.h>

namespace xxx::editor
{
    class Inspector : public Window
    {
    public:
        virtual void draw() override
        {
            if (ImGui::Begin("Inspector"))
            {
                Entity* activedEntity = Context::get().getActivedEntity();
                if (activedEntity)
                {
                    ImGui::Text(activedEntity->getName().c_str());
                    if (ImGui::CollapsingHeader("Transform"))
                    {
                        static osg::Vec3d entityPosition;
                        static osg::Vec3d entityRotation;
                        static osg::Vec3d entityScale;
                        entityPosition = activedEntity->getPosition();
                        entityRotation = activedEntity->getRotation();
                        entityScale = activedEntity->getScale();
                        if (ImGui::DragScalarN("Position", ImGuiDataType_Double, &entityPosition.x(), 3, 0.01))
                        {
                            activedEntity->setPosition(entityPosition);
                        }
                        if (ImGui::DragScalarN("Rotation", ImGuiDataType_Double, &entityRotation.x(), 3))
                        {
                            activedEntity->setRotation(entityRotation);
                        }
                        if (ImGui::DragScalarN("Scale", ImGuiDataType_Double, &entityScale.x(), 3, 0.01))
                        {
                            activedEntity->setScale(entityScale);
                        }
                    }
                    uint32_t componentsCount = activedEntity->getComponentsCount();
                    for (uint32_t i = 0; i < componentsCount; ++i)
                    {
                        Component* component = activedEntity->getComponent(i);
                        if (component->getType() == Component::Type::MeshRenderer)
                            drawMeshRendererGUI(dynamic_cast<MeshRenderer*>(component));
                    }
                }

                static std::vector<std::string> items = {
                    "Test",
                    "SomeThings",
                    "What",
                    "Why",
                    "Where",
                    "How"
                };
                static int currentItem = 0;
                ImGui::ComboWithFilter("ComboFilter", &currentItem, items, 6);
            }
            ImGui::End();
        }
    };
}
