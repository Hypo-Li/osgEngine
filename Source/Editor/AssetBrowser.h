#pragma once
#include "Window.h"
#include <Engine/Core/Context.h>

#include <osg/Texture2D>
#include <osgDB/ReadFile>

#include <filesystem>

namespace xxx::editor
{
    class AssetBrowser : public Window
    {
        static GLenum getTextureId(osg::Texture2D* texture, osg::GraphicsContext* gc)
        {
            if (!texture->getTextureObject(gc->getState()->getContextID()))
                texture->apply(*gc->getState());
            return texture->getTextureObject(gc->getState()->getContextID())->id();
        }
    public:
        AssetBrowser() : Window("Asset Browser")
        {

        }

        virtual void draw() override
        {
            if (!mVisibility)
                return;
            if (ImGui::Begin(mTitle.c_str()))
            {
                static std::filesystem::path currentPath = Context::get().getEngineAssetPath();
                /*static osg::ref_ptr<osg::Texture2D> fileTexture = new osg::Texture2D(osgDB::readImageFile(TEMP_DIR "file.png"));
                static osg::ref_ptr<osg::Texture2D> folderTexture = new osg::Texture2D(osgDB::readImageFile(TEMP_DIR "folder.png"));
                static GLenum fileTextureId = getTextureId(fileTexture, mImGuiCamera->getGraphicsContext());
                static GLenum folderTextureId = getTextureId(folderTexture, mImGuiCamera->getGraphicsContext());*/

                if (currentPath != Context::get().getEngineAssetPath())
                {
                    if (ImGui::Button("<-"))
                        currentPath = currentPath.parent_path();
                }

                static float padding = 16.0f;
                static float thumbnailSize = 80.0f;
                float cellSize = thumbnailSize + padding;

                float panelWidth = ImGui::GetContentRegionAvail().x;
                int columnCount = (int)(panelWidth / cellSize);
                if (columnCount < 1)
                    columnCount = 1;

                ImGuiIO& io = ImGui::GetIO();
                //ImGui::Columns(columnCount, 0, false);

                //for (auto& directoryEntry : std::filesystem::directory_iterator(currentPath))
                //{
                //    const auto& path = directoryEntry.path();
                //    if (directoryEntry.is_regular_file() && path.extension().string() != ".xast")
                //        continue;

                //    std::string stemString = path.stem().string();

                //    /*ImGui::PushID(stemString.c_str());
                //    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                //    ImGui::ImageButton(ImTextureID(directoryEntry.is_directory() ? folderTextureId : fileTextureId), {thumbnailSize, thumbnailSize}, {0, 1}, {1, 0});
                //    ImGui::PopStyleColor();
                //    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                //    {
                //        if (directoryEntry.is_directory())
                //            currentPath /= path.filename();
                //    }

                //    ImGui::TextWrapped(stemString.c_str());

                //    ImGui::NextColumn();

                //    ImGui::PopID();*/

                //    FileIcon(stemString.c_str(), false, ImTextureID(directoryEntry.is_directory() ? folderTextureId : fileTextureId), { thumbnailSize, thumbnailSize }, true, thumbnailSize, thumbnailSize);

                //    ImGui::NextColumn();
                //}

                //ImGui::Columns(1);

                //ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 16, 512);
                //ImGui::SliderFloat("Padding", &padding, 0, 32);

                /*ImGui::BeginTable("asset_browser", columnCount);
                uint32_t entryIndex = 0;
                for (auto& directoryEntry : std::filesystem::directory_iterator(currentPath))
                {
                    const auto& path = directoryEntry.path();
                    if (directoryEntry.is_regular_file() && path.extension().string() != ".xast")
                        continue;

                    if (entryIndex % columnCount == 0)
                        ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(entryIndex % columnCount);

                    std::string stemString = path.stem().string();
                    ImGui::PushID(stemString.c_str());
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    ImGui::ImageButton(ImTextureID(directoryEntry.is_directory() ? folderTextureId : fileTextureId), { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });
                    ImGui::PopStyleColor();
                    ImGui::TextWrapped(stemString.c_str());
                    ImGui::PopID();
                    entryIndex++;
                }

                ImGui::EndTable();*/

            }
            ImGui::End();
        }
    };
}
