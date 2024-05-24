#include <Core/Render/Pipeline.h>
#include <Editor/ImGuiHandler.h>
#include <Core/Base/Entity.h>
#include <Core/Component/MeshRenderer.h>
#include "TestEventHandler.h"
#include <osg/BufferObject>
#include <osg/BufferIndexBinding>
#include <osg/Depth>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgDB/ReadFile>
#include <iomanip>

static const char* inputVS = R"(
#version 430 core
in vec3 osg_Vertex;
in vec3 osg_Normal;
in vec4 osg_Color;
out V2F
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec4 deltaViewSpace;
}v2f;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uProjectionMatrixJittered;
    mat4 uInverseViewMatrix;
    mat4 uInverseProjectionMatrix;
    mat4 uInverseProjectionMatrixJittered;
    mat4 uPreFrameMatrix;
};
layout(location = 0) uniform mat4 uPreFrameModelViewMatrix;
uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ProjectionMatrix;
uniform mat3 osg_NormalMatrix;

void main()
{
    vec4 viewSpace = osg_ModelViewMatrix * vec4(osg_Vertex, 1.0);
    vec4 clipSpace = osg_ProjectionMatrix * viewSpace;
    vec4 preFrameViewSpace = uPreFrameModelViewMatrix * vec4(osg_Vertex, 1.0);
    // delta in view space
    v2f.deltaViewSpace = viewSpace - preFrameViewSpace;
    gl_Position = clipSpace;
    v2f.position = viewSpace.xyz;
    v2f.normal = osg_NormalMatrix * osg_Normal;
    v2f.color = osg_Color;
}
)";

static const char* inputFS = R"(
#version 430 core
in V2F
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec4 deltaViewSpace;
}v2f;
out vec4 fragData[2];

void main()
{
    vec3 viewDir = normalize(-v2f.position);
    vec3 normal = normalize(v2f.normal);
    float ndv = max(dot(normal, viewDir), 0.0);
    fragData[0] = v2f.color;
    fragData[1] = v2f.deltaViewSpace;
}
)";

static const char* quadVS = R"(
#version 430 core
layout(location = 0) in vec3 osg_Vertex;
out vec2 uv;
void main()
{
    gl_Position = vec4(osg_Vertex, 1.0);
    uv = osg_Vertex.xy * 0.5 + 0.5;
}
)";

static const char* taaFS = R"(
#version 430 core
in vec2 uv;
out vec4 fragData;
uniform uint osg_FrameNumber;
uniform sampler2D uCurrentColorTexture;
uniform sampler2D uCurrentDepthTexture;
uniform sampler2D uHistoryColorTexture;
uniform sampler2D uHistoryDepthTexture;
uniform sampler2D uMotionVectorTexture;
uniform vec4 uResolution;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uProjectionMatrixJittered;
    mat4 uInverseViewMatrix;
    mat4 uInverseProjectionMatrix;
    mat4 uInverseProjectionMatrixJittered;
    mat4 uPreFrameMatrix;
};

vec3 RGB2YCoCg(vec3 rgb)
{
    return mat3(
        0.25, 0.5, -0.25,
        0.5, 0.0, 0.5,
        0.25, -0.5, -0.25
    ) * rgb;
}

vec3 YCoCg2RGB(vec3 yCoCg)
{
    return mat3(
        1.0, 1.0, 1.0,
        1.0, 0.0, -1.0,
        -1.0, 1.0, -1.0
    ) * yCoCg;
}

void main()
{
    vec3 currentColor = textureLod(uCurrentColorTexture, uv, 0).rgb;
    if (osg_FrameNumber == 1)
    {
        fragData = vec4(currentColor, 1.0);
        return;
    }

    float fragDepth = textureLod(uCurrentDepthTexture, uv, 0).r;

    /*if (fragDepth == 1.0)
    {
        fragData = vec4(currentColor, 1.0);
        return;
    }*/

    vec4 currentFrameNdcSpaceJittered = vec4(uv * 2.0 - 1.0, fragDepth * 2.0 - 1.0, 1.0);
    vec4 currentFrameViewSpace = uInverseProjectionMatrixJittered * currentFrameNdcSpaceJittered;
    currentFrameViewSpace *= 1.0 / currentFrameViewSpace.w;
    
    vec4 currentFrameNdcSpace = uProjectionMatrix * currentFrameViewSpace;
    currentFrameNdcSpace *= 1.0 / currentFrameNdcSpace.w;

    vec4 motionVector = vec4(textureLod(uMotionVectorTexture, uv, 0).rgb, 0.0);
    vec4 preFrameNdcSpace = uPreFrameMatrix * (currentFrameViewSpace - motionVector);
    preFrameNdcSpace *= 1.0 / preFrameNdcSpace.w;

    float preFrameDepth = textureLod(uHistoryDepthTexture, preFrameNdcSpace.xy * 0.5 + 0.5, 0).r;
    // 还原出的上一帧深度大于实际的上一帧深度, 说明这一片段在上一帧是被覆盖的, 因此不需要混合历史帧
    if (preFrameNdcSpace.z * 2.0 - 1.0 > preFrameDepth)
    {
        fragData = vec4(currentColor, 1.0);
        return;
    }

    vec2 velocity = (currentFrameNdcSpace.xy * 0.5 + 0.5) - (preFrameNdcSpace.xy * 0.5 + 0.5);

    vec2 preFragPos = uv - velocity;
    vec3 historyColor = textureLod(uHistoryColorTexture, clamp(preFragPos, vec2(0.0), vec2(1.0)), 0).rgb;

    vec3 currentYCoCgE = RGB2YCoCg(textureLodOffset(uCurrentColorTexture, uv, 0.0, ivec2(1, 0)).rgb);
    vec3 currentYCoCgN = RGB2YCoCg(textureLodOffset(uCurrentColorTexture, uv, 0.0, ivec2(0, 1)).rgb);
    vec3 currentYCoCgW = RGB2YCoCg(textureLodOffset(uCurrentColorTexture, uv, 0.0, ivec2(-1, 0)).rgb);
    vec3 currentYCoCgS = RGB2YCoCg(textureLodOffset(uCurrentColorTexture, uv, 0.0, ivec2(0, -1)).rgb);
    vec3 historyYCoCg = RGB2YCoCg(historyColor);

    vec3 boxMin = min(currentColor, min(currentYCoCgE, min(currentYCoCgN, min(currentYCoCgW, currentYCoCgS))));
    vec3 boxMax = max(currentColor, max(currentYCoCgE, max(currentYCoCgN, max(currentYCoCgW, currentYCoCgS))));
    
    historyColor = YCoCg2RGB(clamp(historyYCoCg, boxMin, boxMax));

    // vec3 currentColorE = textureOffset(uCurrentColorTexture, uv, ivec2(1, 0)).rgb;
    // vec3 currentColorN = textureOffset(uCurrentColorTexture, uv, ivec2(0, 1)).rgb;
    // vec3 currentColorW = textureOffset(uCurrentColorTexture, uv, ivec2(-1, 0)).rgb;
    // vec3 currentColorS = textureOffset(uCurrentColorTexture, uv, ivec2(0, -1)).rgb;
    
    // vec3 boxMin = min(currentColor, min(currentColorE, min(currentColorN, min(currentColorW, currentColorS))));
    // vec3 boxMax = max(currentColor, max(currentColorE, max(currentColorN, max(currentColorW, currentColorS))));

    // historyColor = clamp(historyColor, boxMin, boxMax);

    fragData = vec4(mix(historyColor, currentColor, vec3(0.05)), 1.0);
}
)";

static const char* updateHistoryFS = R"(
#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler2D uNewHistoryColorTexture;
uniform sampler2D uNewHistoryDepthTexture;
void main()
{
    fragData = texture(uNewHistoryColorTexture, uv);
    gl_FragDepth = texture(uNewHistoryDepthTexture, uv).r;
}
)";

static const char* finalFS = R"(
#version 430 core
in vec2 uv;
out vec4 fragData;
void main()
{
    //fragData = texture(uColorTexture, uv);
}
)";

class MyCameraManipulator : public osgGA::CameraManipulator
{
public:
    MyCameraManipulator() :
        _position(osg::Vec3d()),
        _rotation(osg::Quat()),
        _velocity(0.005)
    {
        calculateVectors();
    }

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
    {
        static bool keyDown[26] = { false };
        switch (ea.getEventType())
        {
        case osgGA::GUIEventAdapter::KEYDOWN:
        case osgGA::GUIEventAdapter::KEYUP:
        {
            using KEY = osgGA::GUIEventAdapter::KeySymbol;
            bool isKeyDown = ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN;
            switch (ea.getKey())
            {
            case KEY::KEY_W:
                keyDown['w' - 'a'] = isKeyDown;
                break;
            case KEY::KEY_A:
                keyDown['a' - 'a'] = isKeyDown;
                break;
            case KEY::KEY_S:
                keyDown['s' - 'a'] = isKeyDown;
                break;
            case KEY::KEY_D:
                keyDown['d' - 'a'] = isKeyDown;
                break;
            default:
                break;
            }
            return true;
        }
        case osgGA::GUIEventAdapter::FRAME:
        {
            if (keyDown['w' - 'a'])
                moveFront(_velocity);
            if (keyDown['a' - 'a'])
                moveRight(-_velocity);
            if (keyDown['s' - 'a'])
                moveFront(-_velocity);
            if (keyDown['d' - 'a'])
                moveRight(_velocity);
            return false;
        }
        default: return false;
        }
    }

    void moveFront(double distance)
    {
        _position += _frontVector * distance;
    }

    void moveRight(double distance)
    {
        _position += _rightVector * distance;
    }

    void moveUp(double distance)
    {
        _position += _upVector * distance;
    }

    virtual void setByMatrix(const osg::Matrixd& matrix)
    {
        _position = matrix.getTrans();
        _rotation = matrix.getRotate();
    }

    virtual void setByInverseMatrix(const osg::Matrixd& matrix)
    {
        setByMatrix(osg::Matrixd::inverse(matrix));
    }

    virtual osg::Matrixd getMatrix() const
    {
        return osg::Matrixd::rotate(_rotation) * osg::Matrixd::translate(_position);
    }

    virtual osg::Matrixd getInverseMatrix() const
    {
        return osg::Matrixd::inverse(getMatrix());
    }

private:
    osg::Vec3d _position;
    osg::Quat _rotation;
    osg::Vec3 _frontVector;
    osg::Vec3 _rightVector;
    osg::Vec3 _upVector;
    double _velocity;

    void calculateVectors()
    {
        _upVector = _rotation * osg::Vec3(0.0, 0.0, 1.0);
        _rightVector = _rotation * osg::Vec3(1.0, 0.0, 0.0);
        _frontVector = _rotation * osg::Vec3(0.0, 1.0, 0.0);
    }
};

struct ViewData
{
    osg::Matrixf viewMatrix;
    osg::Matrixf projectionMatrix;
    osg::Matrixf projectionMatrixJittered;
    osg::Matrixf inverseViewMatrix;
    osg::Matrixf inverseProjectionMatrix;
    osg::Matrixf inverseProjectionMatrixJittered;
    osg::Matrixf preFrameMatrix; // = Proj_pf * View_pf * InvView_cf
    osg::Matrixf preFrameModelViewMatrix;
}gViewData;
osg::ref_ptr<osg::UniformBufferBinding> gViewDataUBB;

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

class MainCameraUpdateCallback : public osg::NodeCallback
{
    osg::ref_ptr<xxx::ImGuiHandler> _imguiHandler;
public:
    MainCameraUpdateCallback(xxx::ImGuiHandler* imguiHandler) : _imguiHandler(imguiHandler) {}
    virtual ~MainCameraUpdateCallback() = default;

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        static int index = 1;
        double x = halton(index, 2), y = halton(index, 3);
        index = index % 16 + 1;

        osg::Camera* camera = node->asCamera();
        assert(camera);
        osg::Matrixd& projectionMatrix = camera->getProjectionMatrix();
        osg::Vec2 viewSize = _imguiHandler->getSceneViewSize();
        projectionMatrix(2, 0) = (x * 2.0 - 1.0) / viewSize.x();
        projectionMatrix(2, 1) = (y * 2.0 - 1.0) / viewSize.y();
        traverse(node, nv);
    }
};

class BallUpdateCallback : public osg::NodeCallback
{
    osg::Vec3 _speed;
    int _axis;
public:
    BallUpdateCallback(osg::Vec3 speed, int axis) : _speed(speed), _axis(axis) {}

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        osg::MatrixTransform* transform = dynamic_cast<osg::MatrixTransform*>(node);
        assert(transform);
        osg::Vec3d position = transform->getMatrix().getTrans();
        position += _speed;
        if (position[_axis] > 10.0 || position[_axis] < -10.0)
        {
            _speed = -_speed;
            position[_axis] = position[_axis] > 0.0 ? 10.0 : -10.0;
        }

        transform->setMatrix(osg::Matrix::translate(position));
    }
};

class DynamicDrawCallback : public osg::Drawable::DrawCallback
{
    osg::Matrixf _preFrameModelViewMatrix;
    osg::ref_ptr<osg::Uniform> _preFrameModelViewMatrixUniform;
public:
    DynamicDrawCallback()
    {
        _preFrameModelViewMatrixUniform = new osg::Uniform(osg::Uniform::FLOAT_MAT4, "uPreFrameModelViewMatrix");
    }

    virtual void drawImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* drawable) const
    {
        // not working, maybe need set uniform by hands
        // set preFrameModelViewMatrix
        _preFrameModelViewMatrixUniform->set(_preFrameModelViewMatrix);
        _preFrameModelViewMatrixUniform->apply(renderInfo.getState()->get<osg::GLExtensions>(), 0);

        // draw
        drawable->drawImplementation(renderInfo);

        // update preFrameModelViewMatrix
        const_cast<osg::Matrixf&>(_preFrameModelViewMatrix) = renderInfo.getState()->getModelViewMatrix();
    }
};

template <typename T>
class DrawCallbackVisitor : public osg::NodeVisitor
{
public:
    DrawCallbackVisitor() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}

    virtual void apply(osg::Drawable& drawable)
    {
        drawable.setDrawCallback(new T);
        traverse(drawable);
    }
};

class InputCameraPreDrawCallback : public osg::Camera::DrawCallback
{
    osg::Matrixd _preFrameViewProjectionMatrix;
    osg::Matrixd _preViewMatrix;
public:
    virtual void operator() (osg::RenderInfo& renderInfo) const
    {
        const osg::Matrixd& viewMatrix = renderInfo.getState()->getInitialViewMatrix();
        const osg::Matrixd& inverseViewMatrix = renderInfo.getState()->getInitialInverseViewMatrix();
        const osg::Matrixd& projectionMatrixJittered = renderInfo.getCurrentCamera()->getProjectionMatrix();
        osg::Matrixd inverseProjectionMatrixJittered = osg::Matrixd::inverse(projectionMatrixJittered);
        osg::Matrixd projectionMatrix = projectionMatrixJittered;
        // remove jitter
        projectionMatrix(2, 0) = 0.0;
        projectionMatrix(2, 1) = 0.0;
        osg::Matrixd inverseProjectionMatrix = osg::Matrixd::inverse(projectionMatrix);

        /*osg::Vec3d eye, center, up;
        _preViewMatrix.getLookAt(eye, center, up);
        std::cout << "PreFrame: " << eye.x() << ", " << eye.y() << ", " << eye.z() << std::endl;
        viewMatrix.getLookAt(eye, center, up);
        std::cout << "CurFrame: " << eye.x() << ", " << eye.y() << ", " << eye.z() << std::endl << std::endl;*/

        gViewData.viewMatrix = viewMatrix;
        gViewData.projectionMatrix = projectionMatrix;
        gViewData.projectionMatrixJittered = projectionMatrixJittered;
        gViewData.inverseViewMatrix = inverseViewMatrix;
        gViewData.inverseProjectionMatrix = inverseProjectionMatrix;
        gViewData.inverseProjectionMatrixJittered = inverseProjectionMatrixJittered;
        gViewData.preFrameMatrix = inverseViewMatrix * _preFrameViewProjectionMatrix;
        osg::FloatArray* buffer = static_cast<osg::FloatArray*>(gViewDataUBB->getBufferData());
        buffer->assign((float*)&gViewData, (float*)(&gViewData + 1));
        buffer->dirty();

        const_cast<osg::Matrixd&>(_preFrameViewProjectionMatrix) = viewMatrix * projectionMatrix;
        const_cast<osg::Matrixd&>(_preViewMatrix) = viewMatrix;
    }
};

int main()
{
    // init global variable
    osg::ref_ptr<osg::FloatArray> viewDataBuffer = new osg::FloatArray((float*)&gViewData, (float*)(&gViewData + 1));
    osg::ref_ptr<osg::UniformBufferObject> viewDataUBO = new osg::UniformBufferObject;
    viewDataBuffer->setBufferObject(viewDataUBO);
    gViewDataUBB = new osg::UniformBufferBinding(0, viewDataBuffer, 0, sizeof(ViewData));

    std::cout.setf(std::ios::fixed);
    std::cout << std::setprecision(10);
    const int width = 1280, height = 800;
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

    osg::Program* program = new osg::Program;
    program->addShader(new osg::Shader(osg::Shader::VERTEX, inputVS));
    program->addShader(new osg::Shader(osg::Shader::FRAGMENT, inputFS));

    DrawCallbackVisitor<DynamicDrawCallback> dcv;
    osg::ref_ptr<osg::Group> rootGroup = new osg::Group;
    osg::Node* meshNode = osgDB::readNodeFile(TEMP_DIR "road.obj");
    meshNode->getOrCreateStateSet()->setAttribute(program, osg::StateAttribute::ON);
    meshNode->accept(dcv);
    osg::Node* redBallMesh = osgDB::readNodeFile(TEMP_DIR "red_ball.obj");
    redBallMesh->getOrCreateStateSet()->setAttribute(program, osg::StateAttribute::ON);
    redBallMesh->accept(dcv);
    osg::Node* greenBallMesh = osgDB::readNodeFile(TEMP_DIR "green_ball.obj");
    greenBallMesh->getOrCreateStateSet()->setAttribute(program, osg::StateAttribute::ON);
    greenBallMesh->accept(dcv);
    osg::Node* blueBallMesh = osgDB::readNodeFile(TEMP_DIR "blue_ball.obj");
    blueBallMesh->getOrCreateStateSet()->setAttribute(program, osg::StateAttribute::ON);
    blueBallMesh->accept(dcv);

    xxx::Entity* entity1 = new xxx::Entity("Suzanne");
    xxx::MeshRenderer* meshRenderer1 = new xxx::MeshRenderer;
    meshRenderer1->setMesh(meshNode);
    entity1->appendComponent(meshRenderer1);
    rootGroup->addChild(entity1->asMatrixTransform());

    xxx::Entity* redBallEntity = new xxx::Entity("RedBall");
    redBallEntity->asMatrixTransform()->addUpdateCallback(new BallUpdateCallback(osg::Vec3(0.01, 0.0, 0.0), 0));
    xxx::MeshRenderer* redBallMeshRenderer = new xxx::MeshRenderer;
    redBallMeshRenderer->setMesh(redBallMesh);
    redBallEntity->appendComponent(redBallMeshRenderer);
    rootGroup->addChild(redBallEntity->asMatrixTransform());

    xxx::Entity* greenBallEntity = new xxx::Entity("GreenBall");
    greenBallEntity->asMatrixTransform()->addUpdateCallback(new BallUpdateCallback(osg::Vec3(0.0, -0.1, 0.0), 1));
    xxx::MeshRenderer* greenBallMeshRenderer = new xxx::MeshRenderer;
    greenBallMeshRenderer->setMesh(greenBallMesh);
    greenBallEntity->appendComponent(greenBallMeshRenderer);
    //rootGroup->addChild(greenBallEntity->asMatrixTransform());

    xxx::Entity* blueBallEntity = new xxx::Entity("BlueBall");
    blueBallEntity->asMatrixTransform()->addUpdateCallback(new BallUpdateCallback(osg::Vec3(0.0, 0.0, 0.2), 2));
    xxx::MeshRenderer* blueBallMeshRenderer = new xxx::MeshRenderer;
    blueBallMeshRenderer->setMesh(blueBallMesh);
    blueBallEntity->appendComponent(blueBallMeshRenderer);
    //rootGroup->addChild(blueBallEntity->asMatrixTransform());

    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
    osg::ref_ptr<osg::Camera> camera = viewer->getCamera();
    camera->setGraphicsContext(gc);
    camera->setViewport(0, 0, width, height);
    camera->setProjectionMatrixAsPerspective(60.0, double(width) / double(height), 0.1, 1000.0);

    using BufferType = xxx::Pipeline::Pass::BufferType;
    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer);
    osg::ref_ptr<xxx::Pipeline::Pass> inputPass = pipeline->addInputPass("Input", 0xFFFFFFFF, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, false, osg::Vec2d(1.0, 1.0));
    inputPass->getCamera()->setClearColor(osg::Vec4(0.2, 0.2, 0.2, 1.0));
    inputPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA8);
    inputPass->attach(BufferType::COLOR_BUFFER1, GL_RGB16F);
    inputPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT, osg::Texture::NEAREST, osg::Texture::NEAREST);
    inputPass->getCamera()->addPreDrawCallback(new InputCameraPreDrawCallback);

    osg::ref_ptr<osg::Texture2D> historyColorTexture = pipeline->createScreenTexture(GL_RGBA8);
    osg::ref_ptr<osg::Texture2D> historyDepthTexture = pipeline->createScreenTexture(GL_DEPTH_COMPONENT, osg::Vec2d(1.0, 1.0), osg::Texture::NEAREST, osg::Texture::NEAREST);

    osg::ref_ptr<osg::Program> taaProgram = new osg::Program;
    taaProgram->addShader(new osg::Shader(osg::Shader::VERTEX, quadVS));
    taaProgram->addShader(new osg::Shader(osg::Shader::FRAGMENT, taaFS));
    osg::ref_ptr<xxx::Pipeline::Pass> taaPass = pipeline->addWorkPass("TAA", taaProgram, GL_COLOR_BUFFER_BIT, false, osg::Vec2d(1.0, 1.0));
    taaPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
    taaPass->applyTexture(inputPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uCurrentColorTexture", 0);
    taaPass->applyTexture(inputPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uCurrentDepthTexture", 1);
    taaPass->applyTexture(historyColorTexture, "uHistoryColorTexture", 2);
    taaPass->applyTexture(historyDepthTexture, "uHistoryDepthTexture", 3);
    taaPass->applyTexture(inputPass->getBufferTexture(BufferType::COLOR_BUFFER1), "uMotionVectorTexture", 4);
    taaPass->setAttribute(gViewDataUBB);

    osg::ref_ptr<osg::Program> updateHistoryProgram = new osg::Program;
    updateHistoryProgram->addShader(new osg::Shader(osg::Shader::VERTEX, quadVS));
    updateHistoryProgram->addShader(new osg::Shader(osg::Shader::FRAGMENT, updateHistoryFS));
    osg::ref_ptr<xxx::Pipeline::Pass> updateHistoryPass = pipeline->addWorkPass("UpdateHistory", updateHistoryProgram, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, false, osg::Vec2(1.0, 1.0));
    updateHistoryPass->attach(BufferType::COLOR_BUFFER0, historyColorTexture);
    updateHistoryPass->attach(BufferType::DEPTH_BUFFER, historyDepthTexture);
    updateHistoryPass->applyTexture(taaPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uNewHistoryColorTexture", 0);
    updateHistoryPass->applyTexture(inputPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uNewHistoryDepthTexture", 1);
    updateHistoryPass->setAttribute(new osg::Depth(osg::Depth::ALWAYS));

    osg::ref_ptr<osg::Program> finalProgram = new osg::Program;
    finalProgram->addShader(new osg::Shader(osg::Shader::VERTEX, quadVS));
    finalProgram->addShader(new osg::Shader(osg::Shader::FRAGMENT, finalFS));
    osg::ref_ptr<xxx::Pipeline::Pass> finalPass = pipeline->addFinalPass("Final", finalProgram);

    viewer->setSceneData(rootGroup);
    viewer->setRealizeOperation(new xxx::ImGuiInitOperation);
    xxx::ImGuiHandler* imguiHandler = new xxx::ImGuiHandler(viewer, finalPass->getCamera(), dynamic_cast<osg::Texture2D*>(taaPass->getBufferTexture(BufferType::COLOR_BUFFER0)), pipeline);
    viewer->getCamera()->addUpdateCallback(new MainCameraUpdateCallback(imguiHandler));
    viewer->addEventHandler(imguiHandler);
    viewer->addEventHandler(new xxx::TestEventHandler(inputPass->getCamera(), imguiHandler));
    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    //viewer->setKeyEventSetsDone(0); // prevent exit when press esc key
    //viewer->addEventHandler(new osgViewer::StatsHandler);
    //viewer->addEventHandler(new osgGA::StateSetManipulator(viewer->getCamera()->getOrCreateStateSet()));
    viewer->realize();
    while (!viewer->done())
    {
        viewer->frame();
    }
    return 0;
}
