#pragma once
#include "Window.h"
#include "Widget.h"
#include <Engine/Core/Entity.h>
#include <Engine/Core/Context.h>
#include <Engine/Component/Light.h>

namespace xxx::editor
{
    class Inspector : public Window
    {
    public:
        Inspector() : Window("Inspector")
        {
            collectComponentClassesRecursively(refl::Reflection::getClass<Component>());
        }

        virtual bool draw() override
        {
            if (!mVisibility)
                return true;

            if (ImGui::Begin(mTitle.c_str(), &mVisibility))
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
                    auto& components = activedEntity->getComponents();
                    int componentId = 0;
                    for (auto componentIt = components.begin(); componentIt != components.end();)
                    {
                        Component* component = *componentIt;
                        if (!ComponentWidget(component, componentId))
                        {
                            componentIt = activedEntity->removeComponent(component);
                            continue;
                        }
                        ++componentIt;
                        ++componentId;
                    }

                    ImGui::NewLine();

                    static refl::Class* previewClass = mComponentClasses.at(0);
                    ComponentClassesCombo("##AddComponent", &previewClass);
                    ImGui::SameLine();
                    if (ImGui::Button("Add Component"))
                    {
                        activedEntity->addComponent(static_cast<Component*>(previewClass->newInstance()));
                    }
                }

                /*static std::vector<std::string> items = {
                    "Test",
                    "SomeThings",
                    "What",
                    "Why",
                    "Where",
                    "How"
                };
                static int currentItem = 0;
                ImGui::ComboWithFilter("ComboFilter", &currentItem, items, 6);*/
            }
            ImGui::End();

            return true;
        }

    protected:
        std::vector<refl::Class*> mComponentClasses;

        void collectComponentClassesRecursively(refl::Class* clazz)
        {
            for (refl::Class* c : clazz->getDerivedClasses())
            {
                if (c->getDefaultObject() != nullptr)
                    mComponentClasses.emplace_back(c);
                collectComponentClassesRecursively(c);
            }
        }

        bool ComponentClassesCombo(const char* label, refl::Class** previewClass)
        {
            bool result = false;
            if (ImGui::BeginCombo(label, std::string((*previewClass)->getName()).c_str()))
            {
                for (refl::Class* clazz : mComponentClasses)
                {
                    const bool is_selected = (clazz == *previewClass);
                    if (ImGui::Selectable(std::string(clazz->getName()).c_str(), is_selected))
                    {
                        *previewClass = clazz;
                        result = true;
                    }

                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }
            return result;
        }
    };
}
