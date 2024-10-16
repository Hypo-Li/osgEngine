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
        Inspector() : Window("Inspector")
        {

        }

        virtual void draw() override
        {
            if (!mVisibility)
                return;
            if (ImGui::Begin(mTitle.c_str()))
            {
                Entity* activedEntity = Context::get().getActivedEntity();
                if (activedEntity)
                {
                    ImGui::Text(activedEntity->getName().c_str());
                    if (ImGui::CollapsingHeader("Transform"))
                    {
                        static osg::Vec3d translation;
                        static osg::Vec3d rotation;
                        static osg::Vec3d scale;
                        translation = activedEntity->getTranslation();
                        rotation = activedEntity->getRotation();
                        scale = activedEntity->getScale();
                        if (ImGui::DragScalarN("Position", ImGuiDataType_Double, &translation.x(), 3, 0.01))
                        {
                            activedEntity->setTranslation(translation);
                        }
                        if (ImGui::DragScalarN("Rotation", ImGuiDataType_Double, &rotation.x(), 3))
                        {
                            activedEntity->setRotation(rotation);
                        }
                        if (ImGui::DragScalarN("Scale", ImGuiDataType_Double, &scale.x(), 3, 0.01))
                        {
                            activedEntity->setScale(scale);
                        }

                       /* osg::Matrixf matrix = activedEntity->getMatrix();
                        osg::Vec3f translation, rotation, scale;
                        ImGuizmo::DecomposeMatrixToComponents(matrix.ptr(), translation.ptr(), rotation.ptr(), scale.ptr());
                        bool modified = ImGui::DragFloat3("Translation", translation.ptr());
                        modified |= ImGui::DragFloat3("Rotation", rotation.ptr());
                        modified |= ImGui::DragFloat3("Scale", scale.ptr());
                        if (modified)
                        {
                            ImGuizmo::RecomposeMatrixFromComponents(translation.ptr(), rotation.ptr(), scale.ptr(), matrix.ptr());
                            activedEntity->setMatrix(matrix);
                        }*/
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
