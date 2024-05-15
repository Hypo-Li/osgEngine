#include <Core/Render/Pipeline.h>
#include <Editor/ImGuiHandler.h>
#include <Core/Base/Entity.h>
#include <Core/Component/MeshRenderer.h>
#include "TestEventHandler.h"
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgDB/ReadFile>

static const char* inputVS = R"(
#version 430 core
in vec3 osg_Vertex;
in vec3 osg_Normal;
in vec4 osg_Color;
out vec3 vo_Position;
out vec3 vo_Normal;
out vec4 vo_Color;
uniform mat4 osg_ModelViewProjectionMatrix;
uniform mat4 osg_ModelViewMatrix;
uniform mat3 osg_NormalMatrix;
void main()
{
    gl_Position = osg_ModelViewProjectionMatrix * vec4(osg_Vertex, 1.0);
    vo_Position = vec3(osg_ModelViewMatrix * vec4(osg_Vertex, 1.0));
    vo_Normal = osg_NormalMatrix * osg_Normal;
    vo_Color = osg_Color;
}
)";

static const char* inputFS = R"(
#version 430 core
in vec3 vo_Position;
in vec3 vo_Normal;
in vec4 vo_Color;
out vec4 fragData;
void main()
{
    vec3 viewDir = normalize(-vo_Position);
    vec3 normal = normalize(vo_Normal);
    float ndv = max(dot(normal, viewDir), 0.0);
    fragData = vo_Color * ndv;
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

static const char* workFS = R"(
#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler2D uColorTexture;
uniform vec4 uResolution;
uniform vec4 uViewport;
void main()
{
    vec2 uvScale = uViewport.zw * uResolution.zw;
    vec2 uvOffset = uViewport.xy * uResolution.zw;
    fragData = texture(uColorTexture, uv * uvScale + uvOffset);
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

int main()
{
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

    osg::ref_ptr<osg::Group> rootGroup = new osg::Group;
    osg::Node* meshNode = osgDB::readNodeFile(TEMP_DIR "suzanne2.obj");
    osg::Program* program = new osg::Program;
    program->addShader(new osg::Shader(osg::Shader::VERTEX, inputVS));
    program->addShader(new osg::Shader(osg::Shader::FRAGMENT, inputFS));
    meshNode->getOrCreateStateSet()->setAttribute(program, osg::StateAttribute::ON);
    //rootGroup->addChild(meshNode);

    xxx::Entity* entity = new xxx::Entity("Suzanne");
    xxx::MeshRenderer* meshRenderer = new xxx::MeshRenderer;
    meshRenderer->setMesh(meshNode);
    entity->appendComponent(meshRenderer);
    rootGroup->addChild(entity->asMatrixTransform());

    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
    osg::ref_ptr<osg::Camera> camera = viewer->getCamera();
    camera->setGraphicsContext(gc);
    camera->setViewport(0, 0, width, height);
    camera->setProjectionMatrixAsPerspective(45.0, double(width) / double(height), 0.1, 1000.0);

    using BufferType = xxx::Pipeline::Pass::BufferType;
    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer);
    osg::ref_ptr<xxx::Pipeline::Pass> inputPass = pipeline->addInputPass("Input", 0xFFFFFFFF, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, false, 2.0, 2.0);
    inputPass->getCamera()->setClearColor(osg::Vec4(0.2, 0.2, 0.2, 1.0));
    inputPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA8);
    inputPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT);

    osg::ref_ptr<osg::Program> workProgram = new osg::Program;
    workProgram->addShader(new osg::Shader(osg::Shader::VERTEX, quadVS));
    workProgram->addShader(new osg::Shader(osg::Shader::FRAGMENT, workFS));
    osg::ref_ptr<xxx::Pipeline::Pass> workPass = pipeline->addWorkPass("Work", workProgram, GL_COLOR_BUFFER_BIT, false, 2.0, 2.0);
    workPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA8);
    workPass->applyTexture(inputPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorTexture", 0);

    osg::ref_ptr<osg::Program> finalProgram = new osg::Program;
    finalProgram->addShader(new osg::Shader(osg::Shader::VERTEX, quadVS));
    finalProgram->addShader(new osg::Shader(osg::Shader::FRAGMENT, finalFS));
    osg::ref_ptr<xxx::Pipeline::Pass> finalPass = pipeline->addFinalPass("Final", finalProgram);

    viewer->setSceneData(rootGroup);
    viewer->setRealizeOperation(new xxx::ImGuiInitOperation);
    viewer->addEventHandler(new xxx::ImGuiHandler(viewer, finalPass->getCamera(), dynamic_cast<osg::Texture2D*>(workPass->getBufferTexture(BufferType::COLOR_BUFFER0)), pipeline));
    viewer->addEventHandler(new xxx::TestEventHandler);
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
