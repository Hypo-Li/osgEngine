#pragma once
#include "Window.h"

#include <osg/Texture2D>

#include <functional>
#include <iostream>

namespace xxx::editor
{
    class SceneView : public Window
    {
    public:
        SceneView(const std::string& title, osg::Texture2D* sceneColorTexture, const std::function<void(int, int)>& resizeCallback, const std::function<void(void)>& getFocusCallback) :
            mTitle(title),
            mViewport(new osg::Viewport),
            mSceneColorTexture(sceneColorTexture),
            mResizeCallback(resizeCallback),
            mGetFocusCallback(getFocusCallback)
        {

        }

        virtual void draw() override
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0, 0.0));
            if (ImGui::Begin(mTitle.c_str()))
            {
                ImGuiIO& io = ImGui::GetIO();
                mIsFocused = ImGui::IsWindowFocused();
                int lastFrameWidth = mViewport->width(), lastFrameHeight = mViewport->height();

                if (mSceneColorTexture->getTextureObject(0))
                {
                    ImGui::Image((void*)mSceneColorTexture->getTextureObject(0)->id(), ImGui::GetContentRegionAvail(), ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
                    mIsHovered = ImGui::IsItemHovered();

                    ImVec2 itemRectMin = ImGui::GetItemRectMin();
                    ImVec2 itemRectMax = ImGui::GetItemRectMax();
                    mViewport->x() = itemRectMin.x;
                    mViewport->y() = io.DisplaySize.y - itemRectMax.y;
                    mViewport->width() = itemRectMax.x - itemRectMin.x;
                    mViewport->height() = itemRectMax.y - itemRectMin.y;
                }
                else
                {
                    mViewport->width() = mViewport->height() = 0;
                }

                if (lastFrameWidth != mViewport->width() || lastFrameHeight != mViewport->height())
                {
                    mResizeCallback(mViewport->width(), mViewport->height());
                }

                if (mIsFocused && mIsHovered)
                    mGetFocusCallback();
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }

        virtual bool isWantCaptureEvents() const override
        {
            return mIsFocused && mIsHovered;
        }

        osg::Viewport* getViewport()
        {
            return mViewport;
        }

    protected:
        std::string mTitle;
        osg::ref_ptr<osg::Viewport> mViewport;
        osg::ref_ptr<osg::Texture2D> mSceneColorTexture;
        std::function<void(int, int)> mResizeCallback;
        std::function<void(void)> mGetFocusCallback;
        bool mIsFocused = false;
        bool mIsHovered = false;
    };
}
