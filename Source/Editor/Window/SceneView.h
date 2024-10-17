#pragma once
#include "Window.h"
#include <Engine/Core/Context.h>
#include <ThirdParty/imgui/ImGuizmo.h>

#include <osg/Texture2D>
#include <osg/Camera>

#include <functional>
#include <iostream>

namespace xxx::editor
{
    class SceneView : public Window
    {
    public:
        SceneView(osg::Camera* camera, osg::Texture2D* sceneColorTexture, const std::function<void(int, int)>& resizeCallback, const std::function<void(void)>& getFocusCallback) :
            Window("Scene View"),
            mViewport(new osg::Viewport),
            mCamera(camera),
            mSceneColorTexture(sceneColorTexture),
            mResizeCallback(resizeCallback),
            mGetFocusCallback(getFocusCallback)
        {

        }

        virtual bool draw() override
        {
            if (!mVisibility)
                return true;

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

                Context& context = Context::get();
                Entity* activedEntity = context.getActivedEntity();
                if (activedEntity)
                {
                    static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::ROTATE);
                    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
                    ImGuizmo::SetGizmoSizeClipSpace(0.2f);
                    ImGuizmo::AllowAxisFlip(false);
                    ImGuizmo::SetDrawlist();

                    if (ImGui::IsKeyPressed(ImGuiKey_G))
                        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
                    if (ImGui::IsKeyPressed(ImGuiKey_R))
                        mCurrentGizmoOperation = ImGuizmo::ROTATE;
                    if (ImGui::IsKeyPressed(ImGuiKey_S))
                        mCurrentGizmoOperation = ImGuizmo::SCALE;
                    if (ImGui::IsKeyPressed(ImGuiKey_L))
                        mCurrentGizmoMode = ImGuizmo::LOCAL;
                    if (ImGui::IsKeyPressed(ImGuiKey_W))
                        mCurrentGizmoMode = ImGuizmo::WORLD;

                    ImGuiIO& io = ImGui::GetIO();

                    ImGuizmo::SetRect(mViewport->x(), io.DisplaySize.y - (mViewport->y() + mViewport->height()), mViewport->width(), mViewport->height());
                    osg::Matrixf viewMatrix = mCamera->getViewMatrix();
                    osg::Matrixf projectionMatrix = mCamera->getProjectionMatrix();
                    osg::Matrixf modelMatrix = activedEntity->getMatrix();
                    if (ImGuizmo::Manipulate(viewMatrix.ptr(), projectionMatrix.ptr(), mCurrentGizmoOperation, mCurrentGizmoOperation == ImGuizmo::SCALE ? ImGuizmo::LOCAL : mCurrentGizmoMode, modelMatrix.ptr()))
                    {
                        activedEntity->setMatrix(modelMatrix);
                    }
                    mUseGizmo = ImGuizmo::IsUsing();
                }
                
            }
            ImGui::End();
            ImGui::PopStyleVar();

            return true;
        }

        virtual bool isWantCaptureEvents() const override
        {
            return mIsFocused && mIsHovered && !mUseGizmo;
        }

        osg::Viewport* getViewport()
        {
            return mViewport;
        }

    protected:
        osg::ref_ptr<osg::Viewport> mViewport;
        osg::ref_ptr<osg::Camera> mCamera;
        osg::ref_ptr<osg::Texture2D> mSceneColorTexture;
        std::function<void(int, int)> mResizeCallback;
        std::function<void(void)> mGetFocusCallback;
        bool mIsFocused = false;
        bool mIsHovered = false;
        bool mUseGizmo = false;
    };
}
