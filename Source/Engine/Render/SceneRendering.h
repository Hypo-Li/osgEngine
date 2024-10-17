#pragma once
#include "Pipeline.h"

#include <osg/BufferIndexBinding>
#include <osgDB/ReadFile>
#include <osg/BlendFunc>
#include <osg/Depth>
namespace xxx
{
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

    Pipeline* createSceneRenderingPipeline(osgViewer::View* view, bool editorMode)
    {
        osg::GraphicsContext* gc = Context::get().getGraphicsContext();
        osg::ref_ptr<osg::FloatArray> viewDataBuffer = new osg::FloatArray(sizeof(ViewData) / sizeof(float));
        osg::ref_ptr<osg::UniformBufferObject> viewDataUBO = new osg::UniformBufferObject;
        viewDataBuffer->setBufferObject(viewDataUBO);
        osg::UniformBufferBinding* viewDataUBB = new osg::UniformBufferBinding(0, viewDataBuffer, 0, sizeof(ViewData));

        using BufferType = Pipeline::Pass::BufferType;
        Pipeline* pipeline = new Pipeline(view, gc);

        Pipeline::Pass* gbufferPass = pipeline->addInputPass("GBuffer", 0x00000001, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gbufferPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);     // Scene Color
        gbufferPass->attach(BufferType::COLOR_BUFFER1, GL_RGB10_A2);    // GBufferA (RGB: Normal)
        gbufferPass->attach(BufferType::COLOR_BUFFER2, GL_RGBA8);       // GBufferB (R: Metallic, G: Specular, B: Roughness, A: ShadingModel)
        gbufferPass->attach(BufferType::COLOR_BUFFER3, GL_RGBA8);       // GBufferC (RGB: BaseColor, A: AO)
        gbufferPass->attach(BufferType::COLOR_BUFFER4, GL_RGBA8);       // GBufferD (RGBA: Custom Data)
        gbufferPass->attach(BufferType::COLOR_BUFFER5, GL_RGB16F);      // Velocity
        gbufferPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT, false, osg::Texture::NEAREST, osg::Texture::NEAREST);
        gbufferPass->getCamera()->addPreDrawCallback(new GBufferPassPreDrawCallback(viewDataUBB));

        osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");

        osg::Program* lightingProgram = new osg::Program;
        lightingProgram->addShader(screenQuadShader);
        lightingProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/Lighting.frag.glsl"));
        Pipeline::Pass* lightingPass = pipeline->addWorkPass("Lighting", lightingProgram, 0);
        lightingPass->attach(BufferType::COLOR_BUFFER0, gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER0));
        lightingPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER1), "uGBufferATexture", 0);
        lightingPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER2), "uGBufferBTexture", 1);
        lightingPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER3), "uGBufferCTexture", 2);
        lightingPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER4), "uGBufferDTexture", 3);
        lightingPass->applyTexture(gbufferPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uDepthTexture", 4);
        lightingPass->setMode(GL_BLEND, osg::StateAttribute::ON);
        lightingPass->setAttribute(new osg::BlendFunc(GL_ONE, GL_SRC_ALPHA));
        lightingPass->setAttribute(viewDataUBB);

        Pipeline::Pass* transparentPass = pipeline->addInputPass("Transparent", 0x00000004, GL_COLOR_BUFFER_BIT);
        transparentPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F, true);
        transparentPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT24, true);
        transparentPass->setAttribute(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false));
        transparentPass->getCamera()->setClearColor(osg::Vec4(0, 0, 0, 0));

        pipeline->addBlitFramebufferCommand(gbufferPass, transparentPass, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        //Pipeline::Pass* taaPass = pipeline->addWorkPass("TAA", );

        osg::Program* combineOpaqueAndTransparentProgram = new osg::Program;
        combineOpaqueAndTransparentProgram->addShader(screenQuadShader);
        combineOpaqueAndTransparentProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CombineOpaqueAndTransparent.frag.glsl"));
        Pipeline::Pass* combineOpaqueAndTransparentPass = pipeline->addWorkPass("CombineOpaqueAndTransparent", combineOpaqueAndTransparentProgram, GL_COLOR_BUFFER_BIT);
        combineOpaqueAndTransparentPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
        combineOpaqueAndTransparentPass->applyTexture(lightingPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uOpaqueColorTexture", 0);
        combineOpaqueAndTransparentPass->applyTexture(transparentPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uTransparentColorTexture", 1);
        combineOpaqueAndTransparentPass->setAttribute(new osg::Depth(osg::Depth::ALWAYS));

        osg::Program* emptyProgram = new osg::Program;
        emptyProgram->addShader(screenQuadShader);
        emptyProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/Empty.frag.glsl"));
        Pipeline::Pass* displayPass = pipeline->addDisplayPass("Display", emptyProgram);

        return pipeline;
    }
}
