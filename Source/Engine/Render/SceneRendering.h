#pragma once
#include "Pipeline.h"
#include "Texture2D.h"
#include "Material.h"

#include <osg/BufferIndexBinding>
#include <osgDB/ReadFile>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/ClipControl>
namespace xxx
{
    struct ViewData
    {
        osg::Matrixf viewMatrix;
        osg::Matrixf inverseViewMatrix;
        osg::Matrixf projectionMatrix;
        osg::Matrixf inverseProjectionMatrix;
        osg::Matrixf reprojectionMatrix;
        osg::Vec2f jitterPixels;
        osg::Vec2f prevJitterPixels;
    };

    class GBufferPassPreDrawCallback : public osg::Camera::DrawCallback
    {
    public:
        GBufferPassPreDrawCallback(osg::UniformBufferBinding* viewDataUBB, osg::Viewport* viewport) :
            mViewDataUBB(viewDataUBB), mViewport(viewport) {}

        virtual void operator () (osg::RenderInfo& renderInfo) const
        {
            osg::FloatArray* buffer = static_cast<osg::FloatArray*>(mViewDataUBB->getBufferData());
            ViewData& viewData = *(ViewData*)(buffer->getDataPointer());

            osg::Matrixd viewMatrix = renderInfo.getCurrentCamera()->getViewMatrix();
            osg::Matrixd inverseViewMatrix = renderInfo.getCurrentCamera()->getInverseViewMatrix();
            osg::Matrixd projectionMatrix = renderInfo.getCurrentCamera()->getProjectionMatrix();
            osg::Matrixd inverseProjectionMatrix = osg::Matrixd::inverse(projectionMatrix);

            // TAA jitter
            {
                uint32_t frameNumberMod8 = renderInfo.getView()->getFrameStamp()->getFrameNumber() % 8;
                double sampleX = halton(frameNumberMod8, 2) - 0.5, sampleY = halton(frameNumberMod8, 3) - 0.5;
                projectionMatrix(2, 0) = 2 * sampleX / mViewport->width();
                projectionMatrix(2, 1) = 2 * sampleY / mViewport->height();
                inverseProjectionMatrix = osg::Matrixd::inverse(projectionMatrix);
                viewData.prevJitterPixels = viewData.jitterPixels;
                viewData.jitterPixels = osg::Vec2(sampleX, sampleY);
            }

            // Reprojection Matrix
            osg::Matrixd reprojectionMatrix;
            {
                reprojectionMatrix = inverseProjectionMatrix * inverseViewMatrix * mPrevFrameViewProjectionMatrix;
                mPrevFrameViewProjectionMatrix = viewMatrix * projectionMatrix;
            }

            viewData.viewMatrix = viewMatrix;
            viewData.inverseViewMatrix = inverseViewMatrix;
            viewData.projectionMatrix = projectionMatrix;
            viewData.inverseProjectionMatrix = inverseProjectionMatrix;
            viewData.reprojectionMatrix = reprojectionMatrix;

            buffer->assign((float*)&viewData, (float*)(&viewData + 1));
            buffer->dirty();
        }

    private:
        osg::ref_ptr<osg::UniformBufferBinding> mViewDataUBB;
        osg::ref_ptr<osg::Viewport> mViewport;
        mutable osg::Matrixd mPrevFrameViewProjectionMatrix;

        static double halton(int index, int base)
        {
            double result = 0.0;
            double f = 1.0 / base;
            int i = index;
            while (i > 0)
            {
                result = result + f * (i % base);
                i = i / base;
                f = f / base;
            }
            return result;
        }
    };

    class PostForwardPassPreDrawCallback : public osg::Camera::DrawCallback
    {
    public:
        PostForwardPassPreDrawCallback(osg::UniformBufferBinding* viewDataUBB) :
            mViewDataUBB(viewDataUBB) {}

        virtual void operator () (osg::RenderInfo& renderInfo) const
        {
            osg::FloatArray* buffer = static_cast<osg::FloatArray*>(mViewDataUBB->getBufferData());
            ViewData& viewData = *(ViewData*)(buffer->getDataPointer());

            viewData.projectionMatrix = renderInfo.getCurrentCamera()->getProjectionMatrix();
            viewData.inverseProjectionMatrix = osg::Matrixf::inverse(viewData.projectionMatrix);

            buffer->assign((float*)&viewData, (float*)(&viewData + 1));
            buffer->dirty();
        }

    private:
        osg::ref_ptr<osg::UniformBufferBinding> mViewDataUBB;
    };

    Pipeline* createSceneRenderingPipeline(osgViewer::View* view, bool editorMode)
    {
        osg::GraphicsContext* gc = Context::get().getGraphicsContext();
        osg::ref_ptr<osg::FloatArray> viewDataBuffer = new osg::FloatArray(sizeof(ViewData) / sizeof(float));
        osg::ref_ptr<osg::UniformBufferObject> viewDataUBO = new osg::UniformBufferObject;
        viewDataBuffer->setBufferObject(viewDataUBO);
        osg::UniformBufferBinding* viewDataUBB = new osg::UniformBufferBinding(0, viewDataBuffer, 0, sizeof(ViewData));

        using BufferType = Pipeline::Pass::BufferType;
        Pipeline* pipeline = new Pipeline(view, gc);

        Pipeline::Pass* gbufferPass = pipeline->addInputPass("GBuffer", GBUFFER_MASK, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gbufferPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);     // Scene Color
        gbufferPass->attach(BufferType::COLOR_BUFFER1, GL_RGB10_A2);    // GBufferA (RGB: Normal)
        gbufferPass->attach(BufferType::COLOR_BUFFER2, GL_RGBA8);       // GBufferB (R: Metallic, G: Specular, B: Roughness, A: ShadingModel)
        gbufferPass->attach(BufferType::COLOR_BUFFER3, GL_RGBA8);       // GBufferC (RGB: BaseColor, A: AO)
        gbufferPass->attach(BufferType::COLOR_BUFFER4, GL_RGBA8);       // GBufferD (RGBA: Custom Data)
        gbufferPass->attach(BufferType::COLOR_BUFFER5, GL_RGB16F);      // Velocity
        gbufferPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT32F, false, osg::Texture::NEAREST, osg::Texture::NEAREST);
        gbufferPass->getCamera()->addPreDrawCallback(new GBufferPassPreDrawCallback(viewDataUBB, gbufferPass->getCamera()->getViewport()));
        gbufferPass->getCamera()->setSmallFeatureCullingPixelSize(4);
        gbufferPass->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
        gbufferPass->setAttribute(new osg::Depth(osg::Depth::GREATER));
        gbufferPass->setAttribute(new osg::ClipControl(osg::ClipControl::LOWER_LEFT, osg::ClipControl::ZERO_TO_ONE));
        gbufferPass->setAttribute(viewDataUBB);
        gbufferPass->getCamera()->setClearDepth(0.0f);

        osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");
        osg::Program* emptyProgram = new osg::Program;
        emptyProgram->addShader(screenQuadShader);
        emptyProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/Empty.frag.glsl"));

        osg::Program* lightingProgram = new osg::Program;
        lightingProgram->addShader(screenQuadShader);
        lightingProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Lighting/Opaque.frag.glsl"));
        Pipeline::Pass* lightingPass = pipeline->addWorkPass("Lighting", lightingProgram, 0);
        lightingPass->attach(BufferType::COLOR_BUFFER0, gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER0));
        lightingPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER1), "uGBufferATexture", 0);
        lightingPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER2), "uGBufferBTexture", 1);
        lightingPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER3), "uGBufferCTexture", 2);
        lightingPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER4), "uGBufferDTexture", 3);
        lightingPass->applyTexture(gbufferPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uDepthTexture", 4);

        lightingPass->applyUniform(new osg::Uniform("uBRDFLutTexture", 5));
        lightingPass->applyUniform(new osg::Uniform("uPrefilterTexture", 6));
        osg::Uniform* enableIBLUniform = new osg::Uniform("uEnableIBL", false);
        lightingPass->applyUniform(enableIBLUniform);
        osg::Uniform* shCoeffUniform = new osg::Uniform(osg::Uniform::FLOAT_VEC4, "uSHCoeff", 9);
        lightingPass->applyUniform(shCoeffUniform);
        osg::Uniform* directionalLightCountUniform = new osg::Uniform("uDirectionalLightCount", uint32_t(0));
        lightingPass->applyUniform(directionalLightCountUniform);

        lightingPass->setMode(GL_BLEND, osg::StateAttribute::ON);
        lightingPass->setAttribute(new osg::BlendFunc(GL_ONE, GL_ONE));
        lightingPass->setAttribute(new osg::Depth(osg::Depth::ALWAYS));
        lightingPass->setAttribute(viewDataUBB);

        Pipeline::Pass* transparentPass = pipeline->addInputPass("Transparent", TRANSPARENT_MASK, GL_COLOR_BUFFER_BIT);
        transparentPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
        transparentPass->attach(BufferType::DEPTH_BUFFER, gbufferPass->getBufferTexture(BufferType::DEPTH_BUFFER));
        transparentPass->setAttribute(new osg::Depth(osg::Depth::GREATER, 0.0, 1.0, false));
        transparentPass->setMode(GL_BLEND);
        transparentPass->setAttribute(new osg::BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA));
        transparentPass->setAttribute(new osg::ClipControl(osg::ClipControl::LOWER_LEFT, osg::ClipControl::ZERO_TO_ONE));
        transparentPass->setAttribute(viewDataUBB);
        transparentPass->getCamera()->setClearColor(osg::Vec4(0, 0, 0, 1));
        transparentPass->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
        transparentPass->applyUniform(new osg::Uniform("uBRDFLutTexture", 14));
        transparentPass->applyUniform(new osg::Uniform("uPrefilterTexture", 15));
        transparentPass->applyUniform(enableIBLUniform);
        transparentPass->applyUniform(shCoeffUniform);
        transparentPass->applyUniform(directionalLightCountUniform);

        osg::Program* combineOpaqueAndTransparentProgram = new osg::Program;
        combineOpaqueAndTransparentProgram->addShader(screenQuadShader);
        combineOpaqueAndTransparentProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CombineOpaqueAndTransparent.frag.glsl"));
        Pipeline::Pass* combineOpaqueAndTransparentPass = pipeline->addWorkPass("CombineOpaqueAndTransparent", combineOpaqueAndTransparentProgram, GL_COLOR_BUFFER_BIT);
        combineOpaqueAndTransparentPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
        combineOpaqueAndTransparentPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uOpaqueColorTexture", 0);
        combineOpaqueAndTransparentPass->applyTexture(transparentPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uTransparentColorTexture", 1);
        combineOpaqueAndTransparentPass->setAttribute(new osg::Depth(osg::Depth::ALWAYS));

        osg::Program* taaProgram = new osg::Program;
        taaProgram->addShader(screenQuadShader);
        taaProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Test/TAA.frag.glsl"));
        Pipeline::Pass* taaPass = pipeline->addWorkPass("TAA", taaProgram, GL_COLOR_BUFFER_BIT);
        taaPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
        taaPass->applyTexture(combineOpaqueAndTransparentPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uCurrentColorTexture", 0);
        taaPass->applyTexture(gbufferPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uCurrentDepthTexture", 1);
        taaPass->setAttribute(new osg::Depth(osg::Depth::ALWAYS));

        osg::Program* copyColorProgram = new osg::Program;
        copyColorProgram->addShader(screenQuadShader);
        copyColorProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CopyColor.frag.glsl"));

        Pipeline::Pass* updateHistoryPass = pipeline->addWorkPass("UpdateHistory", emptyProgram, 0);
        updateHistoryPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
        pipeline->addBlitFramebufferCommand(taaPass, updateHistoryPass, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        taaPass->applyTexture(updateHistoryPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uHistoryColorTexture", 2);

        osg::Program* colorGradingProgram = new osg::Program;
        colorGradingProgram->addShader(screenQuadShader);
        colorGradingProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/ColorGrading.frag.glsl"));
        Pipeline::Pass* colorGradingPass = pipeline->addWorkPass("ColorGrading", colorGradingProgram, GL_COLOR_BUFFER_BIT);
        colorGradingPass->attach(BufferType::COLOR_BUFFER0, gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER0));
        colorGradingPass->attach(BufferType::COLOR_BUFFER1, GL_R8);
        colorGradingPass->applyTexture(taaPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorTexture", 0);
        colorGradingPass->setAttribute(new osg::Depth(osg::Depth::ALWAYS));

        Pipeline::Pass* postForwardPass = pipeline->addInputPass("PostForward", POST_FORWARD_MASK, 0);
        postForwardPass->attach(BufferType::COLOR_BUFFER0, colorGradingPass->getBufferTexture(BufferType::COLOR_BUFFER0));
        postForwardPass->attach(BufferType::COLOR_BUFFER1, colorGradingPass->getBufferTexture(BufferType::COLOR_BUFFER1));
        postForwardPass->attach(BufferType::DEPTH_BUFFER, gbufferPass->getBufferTexture(BufferType::DEPTH_BUFFER));
        postForwardPass->getCamera()->setSmallFeatureCullingPixelSize(4);
        postForwardPass->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
        postForwardPass->setAttribute(new osg::Depth(osg::Depth::GREATER));
        postForwardPass->setAttribute(new osg::ClipControl(osg::ClipControl::LOWER_LEFT, osg::ClipControl::ZERO_TO_ONE));
        postForwardPass->setAttribute(viewDataUBB);
        postForwardPass->getCamera()->setClearDepth(0.0f);
        postForwardPass->setMode(GL_BLEND);
        postForwardPass->getCamera()->addPreDrawCallback(new PostForwardPassPreDrawCallback(viewDataUBB));

        osg::Program* filterProgram = new osg::Program;
        filterProgram->addShader(screenQuadShader);
        filterProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Test/Filter.frag.glsl"));
        Pipeline::Pass* filterPass = pipeline->addWorkPass("Filter", filterProgram, GL_COLOR_BUFFER_BIT);
        filterPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
        filterPass->applyTexture(postForwardPass->getBufferTexture(BufferType::COLOR_BUFFER1), "uMaskTexture", 0);
        filterPass->applyTexture(postForwardPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uCurrentColorTexture", 1);
        filterPass->applyTexture(postForwardPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uCurrentDepthTexture", 2);
        filterPass->setAttribute(viewDataUBB);
        filterPass->setAttribute(new osg::Depth(osg::Depth::ALWAYS));

        Pipeline::Pass* updatePostHistoryPass = pipeline->addWorkPass("UpdatePostHistory", emptyProgram, 0);
        updatePostHistoryPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
        pipeline->addBlitFramebufferCommand(filterPass, updatePostHistoryPass, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        filterPass->applyTexture(updatePostHistoryPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uHistoryColorTexture", 3);
        
        Pipeline::Pass* displayPass = pipeline->addDisplayPass("Display", emptyProgram);

        return pipeline;
    }
}
