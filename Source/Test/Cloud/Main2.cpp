#include <Core/Render/Pipeline.h>
#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osg/OcclusionQueryNode>
#include <osgGA/TrackballManipulator>
#include <osg/BindImageTexture>
#include <osg/BufferIndexBinding>
#include <osgViewer/ViewerEventHandlers>
#include <osg/io_utils>
#include <iomanip>

struct ViewData
{
    osg::Matrixf viewMatrix;
    osg::Matrixf inverseViewMatrix;
    osg::Matrixf projectionMatrix;
    osg::Matrixf inverseProjectionMatrix;
}gViewData;
osg::ref_ptr<osg::UniformBufferBinding> gViewDataUBB;

constexpr uint32_t MAX_SHADOW_CASCADE_COUNT = 4;
uint32_t gShadowCascadeCount = 4;
uint32_t gShadowMapResolution = 2048;
float gCascadeSplitIndex = 3.0f;
osg::Vec3 lightDirection = osg::Vec3(0.7071, 0.0, 0.7071);
float lightFrustumNearOffset = 1000.0f;
float shadowDepthBias = 10.0f;
float shadowSlopeScaleDepthBias = 3.0f;
float receiverBias = 0.1f;
// user
float cascadeBiasDistribution = 1.0f;
float userShadowBias = 0.5f;
float userShadowSlopeBias = 0.5f;

struct ShadowParameters
{
    osg::Matrixf viewSpaceToLightSpaceMatrices[MAX_SHADOW_CASCADE_COUNT];
    osg::Vec4 cascadeParameters[MAX_SHADOW_CASCADE_COUNT]; // (depth bias, slope bias, soft transition scale, cascade far)
    float receiverBias;
}gShadowParameters;
osg::ref_ptr<osg::UniformBufferBinding> gShadowParametersUBB;

class InputPassCameraPreDrawCallback : public osg::Camera::DrawCallback
{
    std::vector<osg::ref_ptr<osg::Camera>> _shadowCastCameras;
public:
    InputPassCameraPreDrawCallback(const std::vector<osg::ref_ptr<osg::Camera>>& shadowCastCameras) : _shadowCastCameras(shadowCastCameras) {}

    virtual void operator() (osg::RenderInfo& renderInfo) const
    {
        // update ViewData
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

        // update ShadowCast Camera's ViewMatrix and ProjectionMatrix
        osg::Matrixd inverseViewProjectionMatrix = osg::Matrixd::inverse(viewMatrix * projectionMatrix);
        double calculatedZNear = projectionMatrix(3, 2) / (projectionMatrix(2, 2) - 1.0);
        double calculatedZFar = projectionMatrix(3, 2) / (projectionMatrix(2, 2) + 1.0);
        double zNear = 0.1/*calculatedZNear*/, zFar = 400.0 /*calculatedZFar*/;

        float sumWeight = (1.0 - std::pow(gCascadeSplitIndex, float(gShadowCascadeCount))) / (1.0 - gCascadeSplitIndex);
        float currentWeight = 0.0;
        double cascadeNear = zNear;

        for (uint32_t i = 0; i < gShadowCascadeCount; ++i)
        {
            currentWeight += std::pow(gCascadeSplitIndex, float(i));
            double cascadeFar = zNear + (currentWeight / sumWeight) * (zFar - zNear);
            double ndcNear = (-cascadeNear * projectionMatrix(2, 2) + projectionMatrix(3, 2)) / (-cascadeNear * projectionMatrix(2, 3) + projectionMatrix(3, 3));
            double ndcFar = (-cascadeFar * projectionMatrix(2, 2) + projectionMatrix(3, 2)) / (-cascadeFar * projectionMatrix(2, 3) + projectionMatrix(3, 3));

            osg::Vec3d worldSpace[8] = {
                osg::Vec3d(-1.0, -1.0, ndcNear) * inverseViewProjectionMatrix,
                osg::Vec3d(-1.0, -1.0, ndcFar) * inverseViewProjectionMatrix,
                osg::Vec3d(-1.0, 1.0, ndcNear) * inverseViewProjectionMatrix,
                osg::Vec3d(-1.0, 1.0, ndcFar) * inverseViewProjectionMatrix,
                osg::Vec3d(1.0, -1.0, ndcNear) * inverseViewProjectionMatrix,
                osg::Vec3d(1.0, -1.0, ndcFar) * inverseViewProjectionMatrix,
                osg::Vec3d(1.0, 1.0, ndcNear) * inverseViewProjectionMatrix,
                osg::Vec3d(1.0, 1.0, ndcFar) * inverseViewProjectionMatrix,
            };
            osg::Vec3d center(0.0, 0.0, 0.0);
            for (uint32_t j = 0; j < 8; ++j)
                center += worldSpace[j];
            center *= 0.125;

            double boundsSphereRadius = 0.0;
            for (uint32_t j = 0; j < 8; ++j)
                boundsSphereRadius = std::max(boundsSphereRadius, (center - worldSpace[j]).length());

            osg::Vec3d eye = center + lightDirection * lightFrustumNearOffset;
            osg::Matrixd lightViewMatrix = osg::Matrixd::lookAt(eye, center, osg::Vec3d(0.0, 0.0, 1.0));

            double minX = std::numeric_limits<double>::max();
            double maxX = std::numeric_limits<double>::lowest();
            double minY = std::numeric_limits<double>::max();
            double maxY = std::numeric_limits<double>::lowest();
            double minZ = std::numeric_limits<double>::max();
            double maxZ = std::numeric_limits<double>::lowest();
            for (uint32_t j = 0; j < 8; j++)
            {
                osg::Vec3d viewSpace = worldSpace[j] * lightViewMatrix;
                minX = std::min(minX, viewSpace.x());
                maxX = std::max(maxX, viewSpace.x());
                minY = std::min(minY, viewSpace.y());
                maxY = std::max(maxY, viewSpace.y());
                minZ = std::min(minZ, viewSpace.z());
                maxZ = std::max(maxZ, viewSpace.z());
            }

            float prevFrameDepth;
            {
                double left, right, bottom, top, zNear, zFar;
                _shadowCastCameras[i]->getProjectionMatrixAsOrtho(left, right, bottom, top, zNear, zFar);
                // 用上一帧的zFar - zNear充当这一帧的光锥体深度
                prevFrameDepth = zFar - zNear;
            }

            float width = maxX - minX, height = maxY - minY;
            const float worldSpaceTexelScaleX = width / gShadowMapResolution;
            const float worldSpaceTexelScaleY = height / gShadowMapResolution;

            minX = std::floor(minX / worldSpaceTexelScaleX) * worldSpaceTexelScaleX;
            maxX = std::floor(maxX / worldSpaceTexelScaleX) * worldSpaceTexelScaleX;

            minY = std::floor(minY / worldSpaceTexelScaleY) * worldSpaceTexelScaleY;
            maxY = std::floor(maxY / worldSpaceTexelScaleY) * worldSpaceTexelScaleY;

            osg::Matrixd lightProjectionMatrix = osg::Matrixd::ortho(minX, minX + width, minY, minY + height, -maxZ, -minZ);
            //osg::Matrixd lightProjectionMatrix = osg::Matrixd::ortho(-boundsSphereRadius, boundsSphereRadius, -boundsSphereRadius, boundsSphereRadius, -maxZ, -minZ);
            // 设置光锥体的viewMatrix和projectionMatrix, 以便于进行剔除
            _shadowCastCameras[i]->setViewMatrix(lightViewMatrix);
            _shadowCastCameras[i]->setProjectionMatrix(lightProjectionMatrix);

            float depthBias = shadowDepthBias / prevFrameDepth;
            const float worldSpaceTexelScale = boundsSphereRadius / gShadowMapResolution;
            depthBias = depthBias * (1.0 - cascadeBiasDistribution) + depthBias * worldSpaceTexelScale * cascadeBiasDistribution;
            depthBias *= userShadowBias;
            float slopeScaleDepthBias = userShadowSlopeBias * shadowSlopeScaleDepthBias;

            depthBias = std::max(depthBias, 0.0f);
            slopeScaleDepthBias = std::max(slopeScaleDepthBias * depthBias, 0.0f);

            float transitionSize = shadowDepthBias / prevFrameDepth;
            transitionSize *= worldSpaceTexelScale;
            transitionSize *= userShadowBias;

            gShadowParameters.cascadeParameters[i] = osg::Vec4(depthBias, slopeScaleDepthBias, 1.0 / transitionSize, cascadeFar);

            cascadeNear = cascadeFar;
        }
        gShadowParameters.receiverBias = receiverBias;

        osg::FloatArray* array = static_cast<osg::FloatArray*>(gShadowParametersUBB->getBufferData());
        array->assign((float*)&gShadowParameters, (float*)(&gShadowParameters + 1));
        array->dirty();
    }
};

class ShadowMaskPassCameraPreDrawCallback : public osg::Camera::DrawCallback
{
    osg::ref_ptr<osg::Camera> _sceneCamera;
    std::vector<osg::ref_ptr<osg::Camera>> _shadowCastCameras;
public:
    ShadowMaskPassCameraPreDrawCallback(osg::Camera* sceneCamera, const std::vector<osg::ref_ptr<osg::Camera>>& shadowCastCameras) : _sceneCamera(sceneCamera), _shadowCastCameras(shadowCastCameras) {}

    virtual void operator() (osg::RenderInfo& renderInfo) const
    {
        for (uint32_t i = 0; i < gShadowCascadeCount; ++i)
            gShadowParameters.viewSpaceToLightSpaceMatrices[i] = _sceneCamera->getInverseViewMatrix() * _shadowCastCameras[i]->getViewMatrix() * _shadowCastCameras[i]->getProjectionMatrix();

        osg::FloatArray* array = static_cast<osg::FloatArray*>(gShadowParametersUBB->getBufferData());
        array->assign((float*)&gShadowParameters, (float*)(&gShadowParameters + 1));
        array->dirty();
    }
};

int main()
{
    osg::setNotifyLevel(osg::FATAL);
    std::cout.setf(std::ios::fixed);
    std::cout << std::setprecision(10);
    osg::ref_ptr<osg::FloatArray> viewDataBuffer = new osg::FloatArray((float*)&gViewData, (float*)(&gViewData + 1));
    osg::ref_ptr<osg::UniformBufferObject> viewDataUBO = new osg::UniformBufferObject;
    viewDataBuffer->setBufferObject(viewDataUBO);
    gViewDataUBB = new osg::UniformBufferBinding(0, viewDataBuffer, 0, sizeof(ViewData));

    osg::ref_ptr<osg::FloatArray> shadowParametersBuffer = new osg::FloatArray((float*)&gShadowParameters, (float*)(&gShadowParameters + 1));
    osg::ref_ptr<osg::UniformBufferObject> shadowParametersUBO = new osg::UniformBufferObject;
    shadowParametersBuffer->setBufferObject(shadowParametersUBO);
    gShadowParametersUBB = new osg::UniformBufferBinding(1, shadowParametersBuffer, 0, sizeof(ShadowParameters));

    const int width = 1280, height = 720;
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
    camera->setProjectionMatrixAsPerspective(90.0, double(width) / double(height), 0.1, 400.0);

    osg::Program* inputProgram = new osg::Program;
    inputProgram->addShader(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/Input2.vert.glsl"));
    inputProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/Input2.frag.glsl"));
    osg::ref_ptr<osg::Node> meshNode = osgDB::readNodeFile(TEMP_DIR "shadow_test.dae");
    meshNode->getOrCreateStateSet()->setAttribute(inputProgram, osg::StateAttribute::ON);
    rootGroup->addChild(meshNode);

    /*osg::ref_ptr<osg::Node> AF1 = osgDB::readNodeFile(R"(D:\Data\Model\noverse\AF1_output.ive)");
    AF1->getOrCreateStateSet()->setAttribute(inputProgram, osg::StateAttribute::ON);
    rootGroup->addChild(AF1);
    osg::ref_ptr<osg::Node> AF2 = osgDB::readNodeFile(R"(D:\Data\Model\noverse\AF2_output.ive)");
    AF2->getOrCreateStateSet()->setAttribute(inputProgram, osg::StateAttribute::ON);
    rootGroup->addChild(AF2);
    osg::ref_ptr<osg::Node> AF3 = osgDB::readNodeFile(R"(D:\Data\Model\noverse\AF3_output.ive)");
    AF3->getOrCreateStateSet()->setAttribute(inputProgram, osg::StateAttribute::ON);
    rootGroup->addChild(AF3);
    osg::ref_ptr<osg::Node> AF4 = osgDB::readNodeFile(R"(D:\Data\Model\noverse\AF4_output.ive)");
    AF4->getOrCreateStateSet()->setAttribute(inputProgram, osg::StateAttribute::ON);
    rootGroup->addChild(AF4);
    osg::ref_ptr<osg::Node> AFD = osgDB::readNodeFile(R"(D:\Data\Model\noverse\AFD_output.ive)");
    AFD->getOrCreateStateSet()->setAttribute(inputProgram, osg::StateAttribute::ON);
    rootGroup->addChild(AFD);
    osg::ref_ptr<osg::Node> BF1 = osgDB::readNodeFile(R"(D:\Data\Model\noverse\BF1_output.ive)");
    BF1->getOrCreateStateSet()->setAttribute(inputProgram, osg::StateAttribute::ON);
    rootGroup->addChild(BF1);
    osg::ref_ptr<osg::Node> BF2 = osgDB::readNodeFile(R"(D:\Data\Model\noverse\BF2_output.ive)");
    BF2->getOrCreateStateSet()->setAttribute(inputProgram, osg::StateAttribute::ON);
    rootGroup->addChild(BF2);
    osg::ref_ptr<osg::Node> BF3 = osgDB::readNodeFile(R"(D:\Data\Model\noverse\BF3_output.ive)");
    BF3->getOrCreateStateSet()->setAttribute(inputProgram, osg::StateAttribute::ON);
    rootGroup->addChild(BF3);
    osg::ref_ptr<osg::Node> BF4 = osgDB::readNodeFile(R"(D:\Data\Model\noverse\BF4_output.ive)");
    BF4->getOrCreateStateSet()->setAttribute(inputProgram, osg::StateAttribute::ON);
    rootGroup->addChild(BF4);
    osg::ref_ptr<osg::Node> BFD = osgDB::readNodeFile(R"(D:\Data\Model\noverse\BFD_output.ive)");
    BFD->getOrCreateStateSet()->setAttribute(inputProgram, osg::StateAttribute::ON);
    rootGroup->addChild(BFD);
    osg::ref_ptr<osg::Node> CF1 = osgDB::readNodeFile(R"(D:\Data\Model\noverse\CF1_output.ive)");
    CF1->getOrCreateStateSet()->setAttribute(inputProgram, osg::StateAttribute::ON);
    rootGroup->addChild(CF1);
    osg::ref_ptr<osg::Node> CF2 = osgDB::readNodeFile(R"(D:\Data\Model\noverse\CF2_output.ive)");
    CF2->getOrCreateStateSet()->setAttribute(inputProgram, osg::StateAttribute::ON);
    rootGroup->addChild(CF2);
    osg::ref_ptr<osg::Node> CF3 = osgDB::readNodeFile(R"(D:\Data\Model\noverse\CF3_output.ive)");
    CF3->getOrCreateStateSet()->setAttribute(inputProgram, osg::StateAttribute::ON);
    rootGroup->addChild(CF3);
    osg::ref_ptr<osg::Node> CFD = osgDB::readNodeFile(R"(D:\Data\Model\noverse\CFD_output.ive)");
    CFD->getOrCreateStateSet()->setAttribute(inputProgram, osg::StateAttribute::ON);
    rootGroup->addChild(CFD);
    osg::ref_ptr<osg::Node> DX = osgDB::readNodeFile(R"(D:\Data\Model\noverse\DX_output.ive)");
    DX->getOrCreateStateSet()->setAttribute(inputProgram, osg::StateAttribute::ON);
    rootGroup->addChild(DX);*/

    using BufferType = xxx::Pipeline::Pass::BufferType;
    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer);
    osg::ref_ptr<xxx::Pipeline::Pass> inputPass = pipeline->addInputPass("Input", 0x00000001, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, false, osg::Vec2d(1.0, 1.0));
    inputPass->getCamera()->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
    inputPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA8);
    inputPass->attach(BufferType::COLOR_BUFFER1, GL_RGB16F, osg::Texture::NEAREST, osg::Texture::NEAREST);
    inputPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT, osg::Texture::NEAREST, osg::Texture::NEAREST);
    inputPass->setAttribute(gViewDataUBB);

    osg::ref_ptr<osg::Shader> screenQuadVertexShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");

    osg::ref_ptr<osg::Texture2D> shadowMapTexture = new osg::Texture2D;
    shadowMapTexture->setTextureSize(gShadowMapResolution * gShadowCascadeCount, gShadowMapResolution);
    shadowMapTexture->setInternalFormat(GL_DEPTH_COMPONENT24);
    shadowMapTexture->setSourceFormat(GL_DEPTH_COMPONENT);
    shadowMapTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    shadowMapTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    osg::ref_ptr<osg::Program> shadowCastProgram = new osg::Program;
    shadowCastProgram->addShader(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Shadow/ShadowCast.vert.glsl"));
    shadowCastProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Shadow/ShadowCast.frag.glsl"));

    std::vector<osg::ref_ptr<xxx::Pipeline::Pass>> shadowCastPasses(gShadowCascadeCount);
    std::vector<osg::ref_ptr<osg::Camera>> shadowCastCameras(gShadowCascadeCount);
    for (uint32_t i = 0; i < gShadowCascadeCount; ++i)
    {
        osg::ref_ptr<xxx::Pipeline::Pass> shadowCastPass = pipeline->addInputPass("ShadowCast" + std::to_string(i), 0x00000002, GL_DEPTH_BUFFER_BIT, true, osg::Vec2(gShadowMapResolution, gShadowMapResolution));
        shadowCastPass->attach(BufferType::DEPTH_BUFFER, shadowMapTexture);
        shadowCastPass->getCamera()->setViewport(i * gShadowMapResolution, 0, gShadowMapResolution, gShadowMapResolution);
        shadowCastPass->getCamera()->setSmallFeatureCullingPixelSize(16.0);
        shadowCastPass->setAttribute(shadowCastProgram, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        shadowCastPass->setAttribute(gShadowParametersUBB);
        shadowCastPass->applyUniform(new osg::Uniform("uCascadeIndex", i));
        shadowCastPasses[i] = shadowCastPass;
        shadowCastCameras[i] = shadowCastPass->getCamera();
    }
    inputPass->getCamera()->setPreDrawCallback(new InputPassCameraPreDrawCallback(shadowCastCameras));

    osg::ref_ptr<osg::Program> shadowMaskProgram = new osg::Program;
    shadowMaskProgram->addShader(screenQuadVertexShader);
    shadowMaskProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Shadow/ShadowMask.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> shadowMaskPass = pipeline->addWorkPass("ShadowMask", shadowMaskProgram, GL_COLOR_BUFFER_BIT, false);
    shadowMaskPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA8);
    shadowMaskPass->getCamera()->setPreDrawCallback(new ShadowMaskPassCameraPreDrawCallback(inputPass->getCamera(), shadowCastCameras));
    shadowMaskPass->applyTexture(inputPass->getBufferTexture(BufferType::COLOR_BUFFER1), "uNormalTexture", 0);
    shadowMaskPass->applyTexture(inputPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uDepthTexture", 1);
    shadowMaskPass->applyTexture(shadowMapTexture, "uShadowMapTexture", 2);
    shadowMaskPass->setAttribute(gViewDataUBB);
    shadowMaskPass->setAttribute(gShadowParametersUBB);

    osg::ref_ptr<osg::Program> copyColorProgram = new osg::Program;
    copyColorProgram->addShader(screenQuadVertexShader);
    copyColorProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/Final.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> finalPass = pipeline->addFinalPass("Final", copyColorProgram);
    finalPass->applyTexture(inputPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorTexture", 0);
    finalPass->applyTexture(shadowMaskPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uShadowMaskTexture", 1);
    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->addEventHandler(new osgViewer::StatsHandler);
    viewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);

    viewer->realize();
    while (!viewer->done())
    {
        viewer->frame();
    }
    return 0;
}
