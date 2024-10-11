#pragma once
#include <Engine/Core/Context.h>
#include <Engine/Core/AssetManager.h>
#include <Engine/Render/Pipeline.h>

#include <osgViewer/Viewer>
#include <osgDB/ReadFile>

namespace xxx
{
    enum class RunMode
    {
        Edit,
        Run,
    };

    struct EngineSetupConfig
    {
        int width, height;
        std::string glContextVersion;
        bool fullScreen;
        RunMode runMode;
    };

    class Engine
    {
    public:
        Engine(osgViewer::View* view, const EngineSetupConfig& setupConfig) :
            mView(view)
        {
            initContext(setupConfig);
            initPipeline(setupConfig);

            Asset* asset = AssetManager::get().getAsset("Engine/TestEntity.xast");
            asset->load();

            mView->setSceneData(dynamic_cast<Entity*>(asset->getRootObject())->getOsgNode());
        }

        osgViewer::View* getView() const
        {
            return mView;
        }

        Pipeline* getPipeline() const
        {
            return mPipeline;
        }

    private:
        osg::ref_ptr<osgViewer::View> mView;
        osg::ref_ptr<Pipeline> mPipeline;

        static osg::GraphicsContext* createGraphicsContext(int width, int height, const std::string& glContextVersion)
        {
            osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits();
            traits->width = width; traits->height = height;
            traits->windowDecoration = true;
            traits->doubleBuffer = true;
            traits->glContextVersion = glContextVersion;
            traits->readDISPLAY();
            traits->setUndefinedScreenDetailsToDefaultScreen();
            osg::GraphicsContext* graphicsContext = osg::GraphicsContext::createGraphicsContext(traits);
            graphicsContext->getState()->setUseModelViewAndProjectionUniforms(true);
            graphicsContext->getState()->setUseVertexAttributeAliasing(true);
            return graphicsContext;
        }

        void initContext(const EngineSetupConfig& setupConfig)
        {
            Context& context = Context::get();
        }

        void initPipeline(const EngineSetupConfig& setupConfig)
        {
            osg::ref_ptr<osg::GraphicsContext> graphicsContext = createGraphicsContext(setupConfig.width, setupConfig.height, setupConfig.glContextVersion);

            osg::ref_ptr<osg::Camera> camera = mView->getCamera();
            camera->setViewport(0, 0, setupConfig.width, setupConfig.height);
            camera->setProjectionMatrixAsPerspective(90.0, double(setupConfig.width) / double(setupConfig.height), 0.1, 400.0);

            mPipeline = new Pipeline(mView, graphicsContext);

            using BufferType = Pipeline::Pass::BufferType;
            Pipeline::Pass* gbufferPass = mPipeline->addInputPass("GBuffer", 0x00000001, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            gbufferPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
            gbufferPass->attach(BufferType::COLOR_BUFFER1, GL_RGBA16F);
            gbufferPass->attach(BufferType::COLOR_BUFFER2, GL_RGBA16F);
            gbufferPass->attach(BufferType::COLOR_BUFFER3, GL_RGBA16F);
            gbufferPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT, false, osg::Texture::NEAREST, osg::Texture::NEAREST);

            osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");
            osg::Program* emptyProgram = new osg::Program;
            emptyProgram->addShader(screenQuadShader);
            emptyProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/Empty.frag.glsl"));
            Pipeline::Pass* displayPass = mPipeline->addDisplayPass("Display", emptyProgram);
        }
    };
}
