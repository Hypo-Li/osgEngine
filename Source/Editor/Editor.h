#pragma once
#include "Window/DockSpace.h"
#include "Window/SceneView.h"
#include "Window/Inspector.h"
#include "Window/AssetBrowser.h"
#include "Window/Hierarchy.h"
#include "Window/WindowManager.h"
#include "ImGuiHandler.h"
#include "PickEventHandler.h"
#include "ControllerManipulator.h"
#include "GLDebugCallback.h"
#include <Engine/Core/Engine.h>
#include <Engine/Core/Scene.h>

#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgUtil/Optimizer>

#include <thread>

namespace xxx::editor
{
    struct EditorSetupConfig
    {

    };

    class EditorRealizeOperation: public osg::Operation
    {
    public:
        EditorRealizeOperation() : osg::Operation("EditorRealizeOperation", false)
        {
            mOperations[0] = new EnableGLDebugOperation;
            mOperations[1] = new ImGuiInitOperation;
        }

        void operator()(osg::Object* object) override
        {
            for (int i = 0; i < mOperations.size(); ++i)
                if (mOperations[i])
                    mOperations[i]->operator()(object);
        }

    protected:
        std::array<osg::ref_ptr<osg::Operation>, 2> mOperations;
    };

    struct MoveUpdateCallback : public osg::NodeCallback
    {
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            constexpr double speed = 100.0;
            constexpr double range = 500.0;
            osg::MatrixTransform* mt = node->asTransform()->asMatrixTransform();
            osg::Vec3d trans = mt->getMatrix().getTrans();
            static double sign = 1;
            if (trans.x() > range)
                sign = -1;
            else if (trans.x() < -range)
                sign = 1;
            trans.x() += sign * speed;
            mt->setMatrix(osg::Matrixd::translate(trans));
            traverse(node, nv);
        }
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
            osg::ref_ptr<osgViewer::View> engineView = new osgViewer::View;
            mEngine = new Engine(engineView, engineSetupConfig);

            mViewer->setRealizeOperation(new EditorRealizeOperation);
            mViewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);

            Pipeline* enginePipeline = mEngine->getPipeline();
            osg::Camera* imguiCamera = enginePipeline->getPass("Display")->getCamera();
            osg::Texture2D* sceneColorTexture = dynamic_cast<osg::Texture2D*>(enginePipeline->getPass("Filter")->getBufferTexture(Pipeline::Pass::BufferType::COLOR_BUFFER0));

            WindowManager& wm = WindowManager::get();
            DockSpace* dockSpace = wm.createWindow<DockSpace>();
            SceneView* sceneView = wm.createWindow<SceneView>(
                mEngine->getView()->getCamera(),
                sceneColorTexture,
                [this](int w, int h) {mEngine->getPipeline()->resize(w, h, true, false); },
                [this]() { if (mViewer->getViewWithFocus() != mEngine->getView()) mViewer->setCameraWithFocus(mEngine->getView()->getCamera()); }
            );
            Inspector* inspector = wm.createWindow<Inspector>();
            AssetBrowser* assetBrowser = wm.createWindow<AssetBrowser>();
            Hierarchy* hierarchy = wm.createWindow<Hierarchy>();

            ImGuiHandler* imguiHandler = new ImGuiHandler(imguiCamera);
            engineView->addEventHandler(imguiHandler);
            engineView->addEventHandler(new PickEventHandler(engineView->getCamera(), sceneView->getViewport()));
            engineView->addEventHandler(new osgViewer::StatsHandler);
            //engineView->setCameraManipulator(new ControllerManipulator(0.05));
            
            mViewer->addView(engineView);
            mViewer->setKeyEventSetsDone(0);
            mViewer->realize();

            Context::get().getGraphicsContext()->makeCurrent();

            const char* version = (const char*)glGetString(GL_VERSION);

            Scene* scene = AssetManager::get().getAsset("Engine/Scene/TestScene")->getRootObjectSafety<Scene>();
            Context::get().setScene(scene);

            engineView->setSceneData(scene->getRootEntity()->getOsgNode());
            osg::MatrixTransform* mt = scene->getRootEntity()->getOsgNode()->asTransform()->asMatrixTransform();
            if (mt)
            {
                mt->addUpdateCallback(new MoveUpdateCallback);
            }

            Context::get().getGraphicsContext()->releaseContext();

            
            
            /*mSimpleView = new osgViewer::View;
            mSimpleView->setSceneData(mEngine->getView()->getSceneData());
            osg::Camera* camera = mSimpleView->getCamera();
            camera->setViewport(0, 0, 128, 128);
            camera->setProjectionMatrixAsPerspective(90.0, 1.0, 0.1, 400.0);

            mSimplePipeline = new Pipeline(mSimpleView, mEngine->getPipeline()->getGraphicsContext());
            Pipeline::Pass* gbufferPass = mSimplePipeline->addInputPass("GBuffer", 0x00000001, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            using BufferType = Pipeline::Pass::BufferType;
            gbufferPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
            gbufferPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT24);

            mSimpleView->addEventHandler(imguiHandler);

            mViewer->addView(mSimpleView);

            SceneView* sceneView2 = wm.createWindow<SceneView>(
                "Scene View 2",
                dynamic_cast<osg::Texture2D*>(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER0)),
                [this](int w, int h) {mSimplePipeline->resize(w, h, true, false); },
                [this]() { if (mViewer->getViewWithFocus() != mSimpleView) mViewer->setCameraWithFocus(mSimpleView->getCamera()); }
            );
            
            mSimpleView->setCameraManipulator(new osgGA::TrackballManipulator);*/
        }

        void run()
        {
            mEngine->run();
        }

        void terminate()
        {
            mEngine->terminate();
        }

    protected:
        Engine* mEngine;
        osg::ref_ptr<osgViewer::CompositeViewer> mViewer;
        //osg::ref_ptr<osgViewer::View> mSimpleView;
        //osg::ref_ptr<Pipeline> mSimplePipeline;

        //void drawSceneView();
        //void drawInspector();
        //void drawHierarchy();
        //void drawAssetBrowser();
    };
}
