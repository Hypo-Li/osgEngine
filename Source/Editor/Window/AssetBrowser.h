#pragma once
#include "Window.h"
#include "MaterialEditor.h"
#include "ShaderEditor.h"
#include <Engine/Core/Context.h>

#include <osg/Texture2D>
#include <osgDB/ReadFile>

#include <filesystem>

namespace xxx::editor
{
    class AssetBrowser : public Window
    {
        static GLenum getTextureId(osg::Texture2D* texture)
        {
            osg::GraphicsContext* gc = Context::get().getGraphicsContext();
            if (!texture->getTextureObject(gc->getState()->getContextID()))
                texture->apply(*gc->getState());
            return texture->getTextureObject(gc->getState()->getContextID())->id();
        }
    public:
        AssetBrowser() : Window("Asset Browser")
        {
            mCurrentPath = Context::get().getEngineAssetPath();

            mFileTexture = new osg::Texture2D(osgDB::readImageFile(TEMP_DIR "file.png"));
            mFolderTexture = new osg::Texture2D(osgDB::readImageFile(TEMP_DIR "folder.png"));
        }

        virtual bool draw() override
        {
            if (!mVisibility)
                return true;

            if (mFileTextureId == GLenum(-1) || mFolderTextureId == GLenum(-1))
            {
                mFileTextureId = getTextureId(mFileTexture);
                mFolderTextureId = getTextureId(mFolderTexture);
            }

            if (ImGui::Begin(mTitle.c_str(), &mVisibility))
            {
                bool isRootPath = mCurrentPath == Context::get().getEngineAssetPath();
                if (isRootPath)
                    ImGui::BeginDisabled();

                if (ImGui::Button("<-"))
                {
                    mCurrentPath = mCurrentPath.parent_path();
                }

                if (isRootPath)
                    ImGui::EndDisabled();

                drawThumbnails();
            }
            ImGui::End();
            return true;
        }

    protected:
        std::filesystem::path mCurrentPath;
        osg::ref_ptr<osg::Texture2D> mFileTexture;
        GLenum mFileTextureId = GLenum(-1);
        osg::ref_ptr<osg::Texture2D> mFolderTexture;
        GLenum mFolderTextureId = GLenum(-1);
        std::stack<std::filesystem::path> mHistoryPathStack;
        std::stack<std::filesystem::path> mForwardPathStack;

        void openAsset(const std::filesystem::path& path)
        {
            std::string assetPath = Asset::convertFullPathToAssetPath(path);
            Asset* asset = AssetManager::get().getAsset(assetPath);

            // find new asset, create it
            if (!asset)
                asset = AssetManager::get().createAsset(nullptr, assetPath);

            if (asset)
            {
                if (asset->getClass() == refl::Reflection::getClass<Material>())
                    WindowManager::get().createWindow<MaterialEditor>(asset);
                else if (asset->getClass() == refl::Reflection::getClass<Shader>())
                    WindowManager::get().createWindow<ShaderEditor>(asset);
            }
            else
            {
                // LogError
            }
        }

        void drawThumbnails()
        {
            static const float padding = 16.0f;
            static const float thumbnailSize = 80.0f;
            float cellSize = thumbnailSize + padding;

            float panelWidth = ImGui::GetContentRegionAvail().x;
            int columnCount = std::max((int)(panelWidth / cellSize), 1);

            std::deque<std::filesystem::path> entries;
            size_t directoryCount = 0;
            for (auto& directoryEntry : std::filesystem::directory_iterator(mCurrentPath))
            {
                const std::filesystem::path& path = directoryEntry.path();
                if (directoryEntry.is_directory())
                {
                    entries.emplace_front(path);
                    directoryCount++;
                }
                else if (directoryEntry.is_regular_file() && path.extension().string() == ".xast")
                    entries.emplace_back(path);
            }

            std::sort(entries.begin(), entries.begin() + directoryCount);
            std::sort(entries.begin() + directoryCount, entries.end());

            ImGui::BeginTable("AssetFileTable", columnCount);
            for (uint32_t i = 0; i < columnCount; ++i)
                ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, cellSize);
            uint32_t entryIndex = 0;
            for (const std::filesystem::path& path : entries)
            {
                bool isDirectory = entryIndex < directoryCount;
                if (entryIndex % columnCount == 0)
                    ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(entryIndex % columnCount);

                std::string stemString = path.stem().string();
                ImGui::PushID(stemString.c_str());
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                ImGui::ImageButton(ImTextureID(isDirectory ? mFolderTextureId : mFileTextureId), { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    if (isDirectory)
                    {
                        mCurrentPath /= path.filename();
                    }
                    else
                    {
                        openAsset(path);
                    }
                }
                ImGui::TextWrapped(stemString.c_str());
                ImGui::PopID();
                entryIndex++;
            }

            ImGui::EndTable();
        }
    };
}
