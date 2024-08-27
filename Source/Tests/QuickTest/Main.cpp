#if 0

#include <Engine/Render/Pipeline.h>
#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osg/MatrixTransform>
#include <osg/BufferObject>
#include <osg/BufferIndexBinding>
#include <osg/Texture3D>
#include <osgViewer/ViewerEventHandlers>

struct ViewData
{
    osg::Matrixf viewMatrix;
    osg::Matrixf inverseViewMatrix;
    osg::Matrixf projectionMatrix;
    osg::Matrixf inverseProjectionMatrix;
}gViewData;
osg::ref_ptr<osg::UniformBufferBinding> gViewDataUBB;

struct AtmosphereParameters
{
    float solarAltitude = 30.0f;
    float solarAzimuth = 0.0f;
    osg::Vec3 sunColor = osg::Vec3(1.0, 1.0, 1.0);
    float sunIntensity = 6.0f;
    float groundRadius = 6360.0f;
    float atmosphereRadius = 6420.0f;
    osg::Vec3 groundAlbedo = osg::Vec3(0.4, 0.4, 0.4);
    osg::Vec3 rayleighScatteringCoeff = osg::Vec3(0.175287, 0.409607, 1.0);
    float rayleighScatteringScale = 0.0331f;
    float rayleighDensityH = 8.0f;
    float mieScatteringBase = 0.003996f;
    float mieAbsorptionBase = 0.000444f;
    float mieDensityH = 1.2f;
    osg::Vec3 ozoneAbsorptionCoeff = osg::Vec3(0.345561, 1.0, 0.045189);
    float ozoneAbsorptionScale = 0.001881f;
    float ozoneCenterHeight = 25.0f;
    float ozoneThickness = 15.0f;
}gAtmosphereParameters;

struct AtmosphereParametersBuffer
{
    osg::Vec3 rayleighScatteringBase;
    float mieScatteringBase;
    osg::Vec3 ozoneAbsorptionBase;
    float mieAbsorptionBase;
    float rayleighDensityH;
    float mieDensityH;
    float ozoneCenterHeight;
    float ozoneThickness;
    osg::Vec3 groundAlbedo;
    float groundRadius;
    osg::Vec3 sunDirection;
    float atmosphereRadius;
    osg::Vec3 sunIntensity;
}gAtmosphereParametersBuffer;
osg::ref_ptr<osg::UniformBufferBinding> gAtmosphereParametersUBB;

void calcAtmosphereParametersBuffer(const AtmosphereParameters& parameter, AtmosphereParametersBuffer& buffer)
{
    buffer.rayleighScatteringBase = parameter.rayleighScatteringCoeff * parameter.rayleighScatteringScale;
    buffer.mieScatteringBase = parameter.mieScatteringBase;
    buffer.ozoneAbsorptionBase = parameter.ozoneAbsorptionCoeff * parameter.ozoneAbsorptionScale;
    buffer.mieAbsorptionBase = parameter.mieAbsorptionBase;
    buffer.rayleighDensityH = parameter.rayleighDensityH;
    buffer.mieDensityH = parameter.mieDensityH;
    buffer.ozoneCenterHeight = parameter.ozoneCenterHeight;
    buffer.ozoneThickness = parameter.ozoneThickness;
    buffer.groundAlbedo = parameter.groundAlbedo;
    buffer.groundRadius = parameter.groundRadius;
    float altitude = osg::DegreesToRadians(parameter.solarAltitude);
    float azimuth = osg::DegreesToRadians(parameter.solarAzimuth);
    buffer.sunDirection = osg::Vec3(
        std::cos(altitude) * std::sin(azimuth),
        std::cos(altitude) * std::cos(azimuth),
        std::sin(altitude)
    );
    buffer.atmosphereRadius = parameter.atmosphereRadius;
    buffer.sunIntensity = parameter.sunColor * parameter.sunIntensity;
}

struct DoubleViewData
{
    osg::Matrixd viewMatrix;
    osg::Matrixd inverseViewMatrix;
    osg::Matrixd projectionMatrix;
    osg::Matrixd inverseProjectionMatrix;
}gDoubleViewData;

class InputPassCameraPreDrawCallback : public osg::Camera::DrawCallback
{
public:
    virtual void operator() (osg::RenderInfo& renderInfo) const
    {
        gDoubleViewData.viewMatrix = renderInfo.getState()->getInitialViewMatrix();
        gDoubleViewData.inverseViewMatrix = renderInfo.getState()->getInitialInverseViewMatrix();
        gDoubleViewData.projectionMatrix = renderInfo.getCurrentCamera()->getProjectionMatrix();
        gDoubleViewData.inverseProjectionMatrix = osg::Matrixd::inverse(gDoubleViewData.projectionMatrix);
        gViewData.viewMatrix = gDoubleViewData.viewMatrix;
        gViewData.inverseViewMatrix = gDoubleViewData.inverseViewMatrix;
        gViewData.projectionMatrix = gDoubleViewData.projectionMatrix;
        gViewData.inverseProjectionMatrix = gDoubleViewData.inverseProjectionMatrix;
        osg::FloatArray* buffer = static_cast<osg::FloatArray*>(gViewDataUBB->getBufferData());
        buffer->assign((float*)&gViewData, (float*)(&gViewData + 1));
        buffer->dirty();
    }
};

static osg::Vec3 positions[] = {
    {0.0, 0.0, 0.0},
    {1.0, 0.0, 0.0},
    {0.0, 1.0, 0.0},
};

static const char* vs = R"(
#version 430 core
in vec4 osg_Vertex;
uniform mat4 osg_ModelViewProjectionMatrix;
void main()
{
    gl_Position = osg_ModelViewProjectionMatrix * osg_Vertex;
}
)";

static const char* fs = R"(
#version 430 core
#pragma import_defines(INPUT_PASS)
out vec4 fragData;
void main()
{
#if (INPUT_PASS == 1)
    fragData = vec4(1, 0, 0, 1);
#elif (INPUT_PASS == 2)
    fragData = vec4(0, 1, 0, 1);
#else
    fragData = vec4(0, 0, 1, 1);
#endif
}
)";

osg::Geometry* createGeometry()
{
    osg::Geometry* geometry = new osg::Geometry;
    geometry->setVertexArray(new osg::Vec3Array(std::begin(positions), std::end(positions)));
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, 3));
    osg::Program* program = new osg::Program;
    program->addShader(new osg::Shader(osg::Shader::VERTEX, vs));
    program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fs));
    osg::StateSet* stateSet = geometry->getOrCreateStateSet();
    stateSet->setAttribute(program, osg::StateAttribute::ON);
    return geometry;
}

osg::Geode* createGeode()
{
    osg::Geode* geode = new osg::Geode;
    osg::Geometry* geometry = createGeometry();
    geode->addDrawable(geometry);
    return geode;
}

class TestCallback : public osg::StateSet::Callback
{
public:
    virtual void operator() (osg::StateSet* ss, osg::NodeVisitor* nv)
    {
        static int currentPass = 1;
        osgGA::EventVisitor* ev = nv->asEventVisitor();
        if (ev)
        {
            osgGA::GUIActionAdapter* aa = ev->getActionAdapter();
            osgGA::EventQueue::Events& events = ev->getEvents();
            for (osgGA::EventQueue::Events::iterator itr = events.begin();
                    itr != events.end(); ++itr)
            {
                osgGA::GUIEventAdapter* ea = dynamic_cast<osgGA::GUIEventAdapter*>(itr->get());
                if (ea)
                {
                    if (ea->getEventType() == osgGA::GUIEventAdapter::KEYDOWN)
                    {
                        using Key = osgGA::GUIEventAdapter::KeySymbol;
                        if (ea->getKey() == Key::KEY_K)
                        {
                            if (currentPass == 1)
                            {
                                ss->setDefine("INPUT_PASS", "2");
                                currentPass = 2;
                            }
                            else
                            {
                                ss->setDefine("INPUT_PASS", "1");
                                currentPass = 1;
                            }
                        }
                    }
                }
            }
        }
        traverse(ss, nv);
    }
};

int main()
{
    osg::ref_ptr<osg::FloatArray> viewDataBuffer = new osg::FloatArray((float*)&gViewData, (float*)(&gViewData + 1));
    osg::ref_ptr<osg::UniformBufferObject> viewDataUBO = new osg::UniformBufferObject;
    viewDataBuffer->setBufferObject(viewDataUBO);
    gViewDataUBB = new osg::UniformBufferBinding(0, viewDataBuffer, 0, sizeof(ViewData));

    calcAtmosphereParametersBuffer(gAtmosphereParameters, gAtmosphereParametersBuffer);
    osg::ref_ptr<osg::FloatArray> atmosphereParametersArray = new osg::FloatArray((float*)&gAtmosphereParametersBuffer, (float*)(&gAtmosphereParametersBuffer + 1));
    osg::ref_ptr<osg::UniformBufferObject> atmosphereParametersUBO = new osg::UniformBufferObject;
    atmosphereParametersArray->setBufferObject(atmosphereParametersUBO);
    gAtmosphereParametersUBB = new osg::UniformBufferBinding(1, atmosphereParametersArray, 0, sizeof(AtmosphereParametersBuffer));

    const int width = 1920, height = 1080;
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

    for (int i = 0; i < 1000; ++i)
        rootGroup->addChild(createGeode());

    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer, gc);
    using BufferType = xxx::Pipeline::Pass::BufferType;
    osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");

    osg::ref_ptr<xxx::Pipeline::Pass> input1Pass = pipeline->addInputPass("Input1", 0x00000001, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    input1Pass->attach(BufferType::COLOR_BUFFER0, GL_RGBA8);
    input1Pass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT, osg::Texture::NEAREST, osg::Texture::NEAREST);
    input1Pass->getCamera()->getOrCreateStateSet()->setDefine("INPUT_PASS", "1");
    input1Pass->getCamera()->getOrCreateStateSet()->setEventCallback(new TestCallback);
    input1Pass->getCamera()->setSmallFeatureCullingPixelSize(100.0f);

    osg::ref_ptr<xxx::Pipeline::Pass> input2Pass = pipeline->addInputPass("Input2", 0x00000002, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    input2Pass->attach(BufferType::COLOR_BUFFER0, GL_RGBA8);
    input2Pass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT, osg::Texture::NEAREST, osg::Texture::NEAREST);
    input2Pass->getCamera()->getOrCreateStateSet()->setDefine("INPUT_PASS", "2");

    /*osg::ref_ptr<osg::Program> transmittanceLutProgram = new osg::Program;
    transmittanceLutProgram->addShader(screenQuadShader);
    transmittanceLutProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Atmosphere/TransmittanceLut.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> transmittanceLutPass = pipeline->addWorkPass("TransmittanceLut", transmittanceLutProgram, 0, true, osg::Vec2(256, 64));
    transmittanceLutPass->attach(BufferType::COLOR_BUFFER0, GL_RGB16F);
    transmittanceLutPass->setAttribute(gAtmosphereParametersUBB, osg::StateAttribute::ON);

    osg::ref_ptr<osg::Texture3D> colorGradingLutTexture = new osg::Texture3D;
    colorGradingLutTexture->setTextureSize(32, 32, 32);
    colorGradingLutTexture->setInternalFormat(GL_RGB16F);
    osg::ref_ptr<osg::Program> colorGradingLutProgram = new osg::Program;
    colorGradingLutProgram->addShader(screenQuadShader);
    colorGradingLutProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/ColorGradingLut.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> colorGradingLut1Pass = pipeline->addWorkPass("ColorGradingLut1", colorGradingLutProgram, 0, true, osg::Vec2(32, 32), true);
    colorGradingLut1Pass->attach(BufferType::COLOR_BUFFER0, colorGradingLutTexture, 0, 0);
    colorGradingLut1Pass->attach(BufferType::COLOR_BUFFER1, colorGradingLutTexture, 0, 1);
    colorGradingLut1Pass->attach(BufferType::COLOR_BUFFER2, colorGradingLutTexture, 0, 2);
    colorGradingLut1Pass->attach(BufferType::COLOR_BUFFER3, colorGradingLutTexture, 0, 3);
    colorGradingLut1Pass->attach(BufferType::COLOR_BUFFER4, colorGradingLutTexture, 0, 4);
    colorGradingLut1Pass->attach(BufferType::COLOR_BUFFER5, colorGradingLutTexture, 0, 5);
    colorGradingLut1Pass->attach(BufferType::COLOR_BUFFER6, colorGradingLutTexture, 0, 6);
    colorGradingLut1Pass->attach(BufferType::COLOR_BUFFER7, colorGradingLutTexture, 0, 7);
    colorGradingLut1Pass->getCamera()->getStateSet()->setDefine("COLOR_GRADING_INDEX", "0");
    osg::ref_ptr<xxx::Pipeline::Pass> colorGradingLut2Pass = pipeline->addWorkPass("ColorGradingLut2", colorGradingLutProgram, 0, true, osg::Vec2(32, 32), true);
    colorGradingLut2Pass->attach(BufferType::COLOR_BUFFER0, colorGradingLutTexture, 0, 8);
    colorGradingLut2Pass->attach(BufferType::COLOR_BUFFER1, colorGradingLutTexture, 0, 9);
    colorGradingLut2Pass->attach(BufferType::COLOR_BUFFER2, colorGradingLutTexture, 0, 10);
    colorGradingLut2Pass->attach(BufferType::COLOR_BUFFER3, colorGradingLutTexture, 0, 11);
    colorGradingLut2Pass->attach(BufferType::COLOR_BUFFER4, colorGradingLutTexture, 0, 12);
    colorGradingLut2Pass->attach(BufferType::COLOR_BUFFER5, colorGradingLutTexture, 0, 13);
    colorGradingLut2Pass->attach(BufferType::COLOR_BUFFER6, colorGradingLutTexture, 0, 14);
    colorGradingLut2Pass->attach(BufferType::COLOR_BUFFER7, colorGradingLutTexture, 0, 15);
    colorGradingLut2Pass->getCamera()->getStateSet()->setDefine("COLOR_GRADING_INDEX", "1");
    osg::ref_ptr<xxx::Pipeline::Pass> colorGradingLut3Pass = pipeline->addWorkPass("ColorGradingLut3", colorGradingLutProgram, 0, true, osg::Vec2(32, 32), true);
    colorGradingLut3Pass->attach(BufferType::COLOR_BUFFER0, colorGradingLutTexture, 0, 16);
    colorGradingLut3Pass->attach(BufferType::COLOR_BUFFER1, colorGradingLutTexture, 0, 17);
    colorGradingLut3Pass->attach(BufferType::COLOR_BUFFER2, colorGradingLutTexture, 0, 18);
    colorGradingLut3Pass->attach(BufferType::COLOR_BUFFER3, colorGradingLutTexture, 0, 19);
    colorGradingLut3Pass->attach(BufferType::COLOR_BUFFER4, colorGradingLutTexture, 0, 20);
    colorGradingLut3Pass->attach(BufferType::COLOR_BUFFER5, colorGradingLutTexture, 0, 21);
    colorGradingLut3Pass->attach(BufferType::COLOR_BUFFER6, colorGradingLutTexture, 0, 22);
    colorGradingLut3Pass->attach(BufferType::COLOR_BUFFER7, colorGradingLutTexture, 0, 23);
    colorGradingLut3Pass->getCamera()->getStateSet()->setDefine("COLOR_GRADING_INDEX", "2");
    osg::ref_ptr<xxx::Pipeline::Pass> colorGradingLut4Pass = pipeline->addWorkPass("ColorGradingLut4", colorGradingLutProgram, 0, true, osg::Vec2(32, 32), true);
    colorGradingLut4Pass->attach(BufferType::COLOR_BUFFER0, colorGradingLutTexture, 0, 24);
    colorGradingLut4Pass->attach(BufferType::COLOR_BUFFER1, colorGradingLutTexture, 0, 25);
    colorGradingLut4Pass->attach(BufferType::COLOR_BUFFER2, colorGradingLutTexture, 0, 26);
    colorGradingLut4Pass->attach(BufferType::COLOR_BUFFER3, colorGradingLutTexture, 0, 27);
    colorGradingLut4Pass->attach(BufferType::COLOR_BUFFER4, colorGradingLutTexture, 0, 28);
    colorGradingLut4Pass->attach(BufferType::COLOR_BUFFER5, colorGradingLutTexture, 0, 29);
    colorGradingLut4Pass->attach(BufferType::COLOR_BUFFER6, colorGradingLutTexture, 0, 30);
    colorGradingLut4Pass->attach(BufferType::COLOR_BUFFER7, colorGradingLutTexture, 0, 31);
    colorGradingLut4Pass->getCamera()->getStateSet()->setDefine("COLOR_GRADING_INDEX", "3");*/

    osg::Program* displayProgram = new osg::Program;
    displayProgram->addShader(screenQuadShader);
    displayProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CopyColor.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> displayPass = pipeline->addDisplayPass("Display", displayProgram);
    displayPass->applyTexture(input1Pass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorTexture", 0);

    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->addEventHandler(new osgViewer::StatsHandler);
    viewer->realize();
    while (!viewer->done())
    {
        viewer->frame();
    }
    return 0;
}

#endif // 0
