#pragma once
#include "Window.h"

namespace xxx::editor
{
    class DockSpace : public Window
    {
    public:
        DockSpace() : Window("DockSpace")
        {

        }

        virtual void draw() override
        {
            static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

            ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

            ImGui::Begin(mTitle.c_str(), nullptr, window_flags);
            ImGui::PopStyleVar(3);
            ImGuiIO& io = ImGui::GetIO();
            if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
            {
                ImGuiID dockspace_id = ImGui::GetID(mTitle.c_str());
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
            }
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("New", "Ctrl+Z")) {}
                    if (ImGui::MenuItem("Open..", "Ctrl+O")) {}
                    if (ImGui::BeginMenu("Open Recent"))
                    {
                        if (ImGui::MenuItem("test.syzproj")) {}
                        ImGui::EndMenu();
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Save", "Ctrl+S")) {}
                    if (ImGui::MenuItem("Save As..", "Shift+Ctrl+S")) {}
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Window"))
                {
                    static bool sceneViewSelected = false;
                    if (ImGui::MenuItem("Scene View", nullptr, &sceneViewSelected)) {}
                    if (ImGui::MenuItem("Inspector")) {}
                    if (ImGui::MenuItem("Asset Browser")) {}
                    ImGui::EndMenu();
                }

                ImGui::EndMenuBar();
            }
            ImGui::End();
        }
    };
}
