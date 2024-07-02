#include <Core/Render/Pipeline.h>
#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osg/DispatchCompute>
#include <osg/Texture3D>
#include <osg/BindImageTexture>
#include <osg/BufferIndexBinding>

struct ViewData
{
    osg::Matrixf viewMatrix;
    osg::Matrixf inverseViewMatrix;
    osg::Matrixf projectionMatrix;
    osg::Matrixf inverseProjectionMatrix;
}gViewData;
osg::ref_ptr<osg::UniformBufferBinding> gViewDataUBB;

class InputPassCameraPreDrawCallback : public osg::Camera::DrawCallback
{
public:
    virtual void operator() (osg::RenderInfo& renderInfo) const
    {
        const osg::Matrixd& viewMatrix = renderInfo.getState()->getInitialViewMatrix();
        const osg::Matrixd& inverseViewMatrix = renderInfo.getState()->getInitialInverseViewMatrix();
        const osg::Matrixd& projectionMatrix = renderInfo.getCurrentCamera()->getProjectionMatrix();
        osg::Matrixd inverseProjectionMatrix = osg::Matrixd::inverse(projectionMatrix);
        gViewData.viewMatrix = viewMatrix;
        gViewData.inverseViewMatrix = inverseViewMatrix;
        gViewData.projectionMatrix = projectionMatrix;
        gViewData.inverseProjectionMatrix = inverseProjectionMatrix;
        osg::FloatArray* buffer = static_cast<osg::FloatArray*>(gViewDataUBB->getBufferData());
        buffer->assign((float*)&gViewData, (float*)(&gViewData + 1));
        buffer->dirty();
    }
};

int main1()
{
    osg::ref_ptr<osg::FloatArray> viewDataBuffer = new osg::FloatArray((float*)&gViewData, (float*)(&gViewData + 1));
    osg::ref_ptr<osg::UniformBufferObject> viewDataUBO = new osg::UniformBufferObject;
    viewDataBuffer->setBufferObject(viewDataUBO);
    gViewDataUBB = new osg::UniformBufferBinding(0, viewDataBuffer, 0, sizeof(ViewData));

    const int width = 1024, height = 1024;
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits();
    traits->width = width; traits->height = height;
    traits->windowDecoration = true;
    traits->doubleBuffer = true;
    traits->glContextVersion = "4.6";
    traits->readDISPLAY();
    traits->setUndefinedScreenDetailsToDefaultScreen();
    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits);
    gc->getState()->setUseModelViewAndProjectionUniforms(true);
    gc->getState()->setUseVertexAttributeAliasing(true);

    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
    osg::ref_ptr<osg::Group> rootGroup = new osg::Group;
    viewer->setSceneData(rootGroup);
    osg::ref_ptr<osg::Camera> camera = viewer->getCamera();
    camera->setGraphicsContext(gc);
    camera->setViewport(0, 0, width, height);
    camera->setProjectionMatrixAsPerspective(60.0, double(width) / double(height), 0.1, 1000.0);

    osg::ref_ptr<osg::Texture3D> noise1Texture = new osg::Texture3D;
    noise1Texture->setTextureSize(128, 128, 128);
    noise1Texture->setInternalFormat(GL_RGBA8);
    noise1Texture->setSourceFormat(GL_RGBA);
    noise1Texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    noise1Texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    noise1Texture->setWrap(osg::Texture::WRAP_R, osg::Texture::REPEAT);
    noise1Texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    noise1Texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    osg::ref_ptr<osg::BindImageTexture> noise1Image = new osg::BindImageTexture(0, noise1Texture, osg::BindImageTexture::WRITE_ONLY, GL_RGBA8);
    osg::ref_ptr<osg::Program> noise1Program = new osg::Program;
    noise1Program->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "VolumetricCloud/Noise1.comp.glsl"));
    osg::ref_ptr<osg::DispatchCompute> noise1Dispatch = new osg::DispatchCompute(32, 32, 2);
    noise1Dispatch->getOrCreateStateSet()->setAttribute(noise1Program, osg::StateAttribute::ON);
    noise1Dispatch->getOrCreateStateSet()->setAttribute(noise1Image, osg::StateAttribute::ON);
    rootGroup->addChild(noise1Dispatch);

    osg::ref_ptr<osg::Texture3D> noise2Texture = new osg::Texture3D;
    noise2Texture->setTextureSize(32, 32, 32);
    noise2Texture->setInternalFormat(GL_RGBA8);
    noise2Texture->setSourceFormat(GL_RGBA);
    noise2Texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    noise2Texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    noise2Texture->setWrap(osg::Texture::WRAP_R, osg::Texture::REPEAT);
    noise2Texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    noise2Texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    osg::ref_ptr<osg::BindImageTexture> noise2Image = new osg::BindImageTexture(0, noise2Texture, osg::BindImageTexture::WRITE_ONLY, GL_RGBA8);
    osg::ref_ptr<osg::Program> noise2Program = new osg::Program;
    noise2Program->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "VolumetricCloud/Noise2.comp.glsl"));
    osg::ref_ptr<osg::DispatchCompute> noise2Dispatch = new osg::DispatchCompute(8, 8, 1);
    noise2Dispatch->getOrCreateStateSet()->setAttribute(noise2Program, osg::StateAttribute::ON);
    noise2Dispatch->getOrCreateStateSet()->setAttribute(noise2Image, osg::StateAttribute::ON);
    rootGroup->addChild(noise2Dispatch);

    /*osg::ref_ptr<osg::Texture2D> noise3Texture = new osg::Texture2D;
    noise3Texture->setTextureSize(128, 128);
    noise3Texture->setInternalFormat(GL_RGBA8);
    noise3Texture->setSourceFormat(GL_RGBA);
    noise3Texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    noise3Texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    noise3Texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    noise3Texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    osg::ref_ptr<osg::BindImageTexture> noise3Image = new osg::BindImageTexture(0, noise3Texture, osg::BindImageTexture::WRITE_ONLY, GL_RGBA8);
    osg::ref_ptr<osg::Program> noise3Program = new osg::Program;
    noise3Program->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "VolumetricClouds/Noise3.comp.glsl"));
    osg::ref_ptr<osg::DispatchCompute> noise3Dispatch = new osg::DispatchCompute();
    noise3Dispatch->getOrCreateStateSet()->setAttribute(noise3Program, osg::StateAttribute::ON);
    noise3Dispatch->getOrCreateStateSet()->setAttribute(noise3Image, osg::StateAttribute::ON);
    rootGroup->addChild(noise3Dispatch);*/

    osg::Program* inputProgram = new osg::Program;
    inputProgram->addShader(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/TestInput.vert.glsl"));
    inputProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/TestInput.frag.glsl"));
    osg::ref_ptr<osg::Node> meshNode = osgDB::readNodeFile(TEMP_DIR "suzanne.obj");
    meshNode->getOrCreateStateSet()->setAttribute(inputProgram, osg::StateAttribute::ON);
    rootGroup->addChild(meshNode);

    using BufferType = xxx::Pipeline::Pass::BufferType;
    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer);
    osg::ref_ptr<xxx::Pipeline::Pass> inputPass = pipeline->addInputPass("Input", 0xFFFFFFFF, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, false, osg::Vec2d(1.0, 1.0));
    //inputPass->getCamera()->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
    inputPass->getCamera()->setClearColor(osg::Vec4(0.09, 0.33, 0.81, 1.0));
    inputPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA8);
    inputPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT, osg::Texture::NEAREST, osg::Texture::NEAREST);
    inputPass->getCamera()->setPreDrawCallback(new InputPassCameraPreDrawCallback);

    osg::ref_ptr<osg::Shader> screenQuadVertexShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");

    osg::ref_ptr<osg::Program> noiseVisualizeProgram = new osg::Program;
    noiseVisualizeProgram->addShader(screenQuadVertexShader);
    noiseVisualizeProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "VolumetricCloud/RayMarching.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> workPass = pipeline->addWorkPass("Work", noiseVisualizeProgram);
    workPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA32F);
    workPass->applyTexture(noise1Texture, "uNoise1Texture", 0);
    workPass->applyTexture(noise2Texture, "uNoise2Texture", 1);
    workPass->applyTexture(inputPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorTexture", 2);
    workPass->applyTexture(inputPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uDepthTexture", 3);
    workPass->setAttribute(gViewDataUBB);

    osg::ref_ptr<osg::Program> copyColorProgram = new osg::Program;
    copyColorProgram->addShader(screenQuadVertexShader);
    copyColorProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CopyColor.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> finalPass = pipeline->addFinalPass("Final", copyColorProgram);
    finalPass->applyTexture(workPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorTexture", 0);

    viewer->setCameraManipulator(new osgGA::TrackballManipulator);

    viewer->realize();
    while (!viewer->done())
    {
        viewer->frame();
    }
    return 0;
}
