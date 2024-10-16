#pragma once
#include <Engine/Core/Context.h>
#include <Engine/Core/AssetManager.h>
#include <Engine/Render/Pipeline.h>

#include <osg/BlendFunc>
#include <osg/BufferIndexBinding>
#include <osgViewer/View>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>

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

    struct ViewData
    {
        osg::Matrixf viewMatrix;
        osg::Matrixf inverseViewMatrix;
        osg::Matrixf projectionMatrix;
        osg::Matrixf inverseProjectionMatrix;
    };

    class GBufferPassPreDrawCallback : public osg::Camera::DrawCallback
    {
    public:
        GBufferPassPreDrawCallback(osg::UniformBufferBinding* viewDataUBB) :
            mViewDataUBB(viewDataUBB)
        {}

        virtual void operator () (osg::RenderInfo& renderInfo) const
        {
            osg::FloatArray* buffer = static_cast<osg::FloatArray*>(mViewDataUBB->getBufferData());
            ViewData& viewData = *(ViewData*)(buffer->getDataPointer());

            viewData.viewMatrix = renderInfo.getCurrentCamera()->getViewMatrix();
            viewData.inverseViewMatrix = renderInfo.getCurrentCamera()->getInverseViewMatrix();
            viewData.projectionMatrix = renderInfo.getCurrentCamera()->getProjectionMatrix();
            viewData.inverseProjectionMatrix = osg::Matrixf::inverse(viewData.projectionMatrix);

            buffer->assign((float*)&viewData, (float*)(&viewData + 1));
            buffer->dirty();
        }

    private:
        osg::ref_ptr<osg::UniformBufferBinding> mViewDataUBB;
    };

    class ResizedCallback : public osg::GraphicsContext::ResizedCallback
    {
        osg::ref_ptr<xxx::Pipeline> mPipeline;
        int mWidth, mHeight;
    public:
        ResizedCallback(xxx::Pipeline* pipeline) : mPipeline(pipeline)
        {
            osg::Viewport* viewport = pipeline->getView()->getCamera()->getViewport();
            mWidth = viewport->width();
            mHeight = viewport->height();
        }

        virtual void resizedImplementation(osg::GraphicsContext* gc, int x, int y, int width, int height)
        {
            if ((width == mWidth && height == mHeight) || (width == 1 && height == 1))
                return;
            mWidth = width, mHeight = height;
            mPipeline->resize(width, height, true);
        }
    };

    class Engine
    {
    public:
        Engine(const EngineSetupConfig& setupConfig) :
            mView(new osgViewer::View)
        {
            initContext(setupConfig);
            initRenderingData();
            initPipeline(setupConfig);

            Asset* asset = AssetManager::get().getAsset("Engine/TestEntity.xast");
            asset->load();

            mView->setSceneData(dynamic_cast<Entity*>(asset->getRootObject())->getOsgNode());
            mView->setCameraManipulator(new osgGA::TrackballManipulator);
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
        osg::ref_ptr<osg::UniformBufferBinding> mViewDataUBB;

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
            context.setGraphicsContext(createGraphicsContext(setupConfig.width, setupConfig.height, setupConfig.glContextVersion));

        }

        void initRenderingData()
        {
            osg::ref_ptr<osg::FloatArray> viewDataBuffer = new osg::FloatArray(sizeof(ViewData) / sizeof(float));
            osg::ref_ptr<osg::UniformBufferObject> viewDataUBO = new osg::UniformBufferObject;
            viewDataBuffer->setBufferObject(viewDataUBO);
            mViewDataUBB = new osg::UniformBufferBinding(0, viewDataBuffer, 0, sizeof(ViewData));
        }

        void initPipeline(const EngineSetupConfig& setupConfig)
        {
            osg::ref_ptr<osg::Camera> camera = mView->getCamera();
            camera->setViewport(0, 0, setupConfig.width, setupConfig.height);
            camera->setProjectionMatrixAsPerspective(90.0, double(setupConfig.width) / double(setupConfig.height), 0.1, 400.0);

            osg::GraphicsContext* gc = Context::get().getGraphicsContext();
            // TODO: break circular reference
            mPipeline = new Pipeline(mView, gc);
            gc->setResizedCallback(new ResizedCallback(mPipeline));

            using BufferType = Pipeline::Pass::BufferType;
            Pipeline::Pass* gbufferPass = mPipeline->addInputPass("GBuffer", 0x00000001, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            gbufferPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);     // Scene Color
            gbufferPass->attach(BufferType::COLOR_BUFFER1, GL_RGB10_A2);    // GBufferA (RGB: Normal)
            gbufferPass->attach(BufferType::COLOR_BUFFER2, GL_RGBA8);       // GBufferB (R: Metallic, G: Specular, B: Roughness, A: ShadingModel)
            gbufferPass->attach(BufferType::COLOR_BUFFER3, GL_RGBA8);       // GBufferC (RGB: BaseColor, A: AO)
            gbufferPass->attach(BufferType::COLOR_BUFFER4, GL_RGBA8);       // GBufferD (RGBA: Custom Data)
            gbufferPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT, false, osg::Texture::NEAREST, osg::Texture::NEAREST);
            gbufferPass->getCamera()->addPreDrawCallback(new GBufferPassPreDrawCallback(mViewDataUBB));

            osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");

            osg::Program* lightingProgram = new osg::Program;
            lightingProgram->addShader(screenQuadShader);
            lightingProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/Lighting.frag.glsl"));
            Pipeline::Pass* lightingPass = mPipeline->addWorkPass("Lighting", lightingProgram, 0);
            lightingPass->attach(BufferType::COLOR_BUFFER0, gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER0));
            lightingPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER1), "uGBufferATexture", 0);
            lightingPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER2), "uGBufferBTexture", 1);
            lightingPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER3), "uGBufferCTexture", 2);
            lightingPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER4), "uGBufferDTexture", 3);
            lightingPass->applyTexture(gbufferPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uDepthTexture", 4);
            lightingPass->setMode(GL_BLEND, osg::StateAttribute::ON);
            lightingPass->setAttribute(new osg::BlendFunc(GL_ONE, GL_SRC_ALPHA));
            lightingPass->setAttribute(mViewDataUBB);

            osg::Program* emptyProgram = new osg::Program;
            emptyProgram->addShader(screenQuadShader);
            emptyProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/Empty.frag.glsl"));
            Pipeline::Pass* displayPass = mPipeline->addDisplayPass("Display", emptyProgram);
        }
    };
}
