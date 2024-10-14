#pragma once
#include "DockSpace.h"
#include "SceneView.h"
#include "Inspector.h"
#include "AssetBrowser.h"
#include "WindowManager.h"
#include "ImGuiHandler.h"
#include "PickEventHandler.h"
#include <Engine/Core/Engine.h>

#include <osgViewer/CompositeViewer>

namespace xxx::editor
{
    struct EditorSetupConfig
    {

    };

    class Editor
    {
    public:
        Editor()
        {
            mViewer = new osgViewer::CompositeViewer;

            EngineSetupConfig engineSetupConfig;
            engineSetupConfig.width = 1024;
            engineSetupConfig.height = 768;
            engineSetupConfig.glContextVersion = "4.6";
            engineSetupConfig.fullScreen = false;
            engineSetupConfig.runMode = RunMode::Edit;
            mEngine = new Engine(engineSetupConfig);

            mViewer->setRealizeOperation(new ImGuiInitOperation);
            mViewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);

            Pipeline* enginePipeline = mEngine->getPipeline();
            uint32_t passCount = enginePipeline->getPassCount();
            osg::Camera* imguiCamera = enginePipeline->getPass(passCount - 1)->getCamera();
            osg::Texture2D* sceneColorTexture = dynamic_cast<osg::Texture2D*>(enginePipeline->getPass(passCount - 2)->getBufferTexture(Pipeline::Pass::BufferType::COLOR_BUFFER0));

            WindowManager& wm = WindowManager::get();
            DockSpace* dockSpace = wm.createWindow<DockSpace>();
            SceneView* sceneView = wm.createWindow<SceneView>(
                "Scene View",
                sceneColorTexture,
                [this](int w, int h) {mEngine->getPipeline()->resize(w, h, false); },
                [this]() { if (mViewer->getViewWithFocus() != mEngine->getView()) mViewer->setCameraWithFocus(mEngine->getView()->getCamera()); }
            );
            Inspector* inspector = wm.createWindow<Inspector>();
            AssetBrowser* assetBrowser = wm.createWindow<AssetBrowser>();

            osgViewer::View* engineView = mEngine->getView();
            engineView->addEventHandler(new ImGuiHandler(imguiCamera));
            engineView->addEventHandler(new PickEventHandler(engineView->getCamera(), sceneView->getViewport()));

            mViewer->addView(engineView);

            mSimpleView = new osgViewer::View;
            mSimpleView->setSceneData(mEngine->getView()->getSceneData());
            osg::Camera* camera = mSimpleView->getCamera();
            camera->setViewport(0, 0, 128, 128);
            camera->setProjectionMatrixAsPerspective(90.0, 1.0, 0.1, 400.0);
            camera->setAllowEventFocus(true);

            mSimplePipeline = new Pipeline(mSimpleView, mEngine->getPipeline()->getGraphicsContext());
            Pipeline::Pass* gbufferPass = mSimplePipeline->addInputPass("GBuffer", 0x00000001, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            using BufferType = Pipeline::Pass::BufferType;
            gbufferPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
            gbufferPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT24);

            mViewer->addView(mSimpleView);

            SceneView* sceneView2 = wm.createWindow<SceneView>(
                "Scene View 2",
                dynamic_cast<osg::Texture2D*>(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER0)),
                [this](int w, int h) {mSimplePipeline->resize(w, h, false); },
                [this]() { if (mViewer->getViewWithFocus() != mSimpleView) mViewer->setCameraWithFocus(mSimpleView->getCamera()); }
            );

            mSimpleView->setCameraManipulator(new osgGA::TrackballManipulator);

        }

        void run()
        {
            mViewer->realize();
            while (!mViewer->done())
            {
                mViewer->frame();
            }
        }

    protected:
        Engine* mEngine;
        osg::ref_ptr<osgViewer::CompositeViewer> mViewer;
        osg::ref_ptr<osgViewer::View> mSimpleView;
        osg::ref_ptr<Pipeline> mSimplePipeline;

        //void drawSceneView();
        //void drawInspector();
        //void drawHierarchy();
        //void drawAssetBrowser();
    };
}
