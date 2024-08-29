#include "Pipeline.h"

namespace xxx
{
    MyTexture2DMultisample::MyTexture2DMultisample() : osg::Texture2DMultisample()
    {}

    MyTexture2DMultisample::MyTexture2DMultisample(GLsizei numSamples, GLboolean fixedsamplelocations) :
        osg::Texture2DMultisample(numSamples, fixedsamplelocations)
    {}

    void MyTexture2DMultisample::apply(osg::State& state) const
    {
        // current OpenGL context.
        const unsigned int contextID = state.getContextID();
        const osg::GLExtensions* extensions = state.get<osg::GLExtensions>();
        if (!extensions->isTextureMultisampledSupported)
        {
            OSG_INFO << "Texture2DMultisample not supported." << std::endl;
            return;
        }

        // get the texture object for the current contextID.
        TextureObject* textureObject = getTextureObject(contextID);

        if (textureObject)
        {
            textureObject->bind();
        }
        else if ((_textureWidth != 0) && (_textureHeight != 0) && (_numSamples != 0))
        {
            // no image present, but dimensions at set so lets create the texture
            /*GLenum texStorageSizedInternalFormat = extensions->isTextureStorageEnabled && (_borderWidth == 0) ? selectSizedInternalFormat() : 0;
            if (texStorageSizedInternalFormat != 0)
            {
                textureObject = generateAndAssignTextureObject(contextID, getTextureTarget(), 1, texStorageSizedInternalFormat, _textureWidth, _textureHeight, 1, 0);
                textureObject->bind();

                extensions->glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, _numSamples, texStorageSizedInternalFormat, _textureWidth, _textureHeight, _fixedsamplelocations);
            }
            else*/
            {
                textureObject = generateAndAssignTextureObject(contextID, getTextureTarget(), 1, _internalFormat, _textureWidth, _textureHeight, 1, _borderWidth);
                textureObject->bind();

                extensions->glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
                                                 _numSamples,
                                                 _internalFormat,
                                                 _textureWidth,
                                                 _textureHeight,
                                                 _fixedsamplelocations);
            }

        }
        else
        {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        }
    }

    Pipeline::Pass* Pipeline::addInputPass(const std::string& name, osg::Node::NodeMask cullMask, GLbitfield clearMask, bool fixedSize, osg::Vec2 sizeScale)
    {
        osg::Camera* camera = new osg::Camera;
        camera->setName(name);
        camera->setGraphicsContext(_graphicsContext);
        camera->setCullMask(cullMask);
        camera->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
        camera->setClearMask(clearMask);
        camera->setRenderOrder(osg::Camera::PRE_RENDER);
        camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        camera->setImplicitBufferAttachmentMask(0, 0);
        if (fixedSize)
        {
            camera->setViewport(0, 0, sizeScale.x(), sizeScale.y());
            camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        }
        else
        {
            osg::Viewport* nativeViewport = _view->getCamera()->getViewport();
            camera->setViewport(nativeViewport->x(), nativeViewport->y(), nativeViewport->width() * sizeScale.x(), nativeViewport->height() * sizeScale.y());
        }

        _view->addSlave(camera, true);
        Pass* newPass = new Pass(camera, fixedSize, sizeScale);
        _passes.push_back(newPass);
        return newPass;
    }

    class RunOnceCullCallback : public osg::NodeCallback
    {
    public:
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            node->setNodeMask(0);
            traverse(node, nv);
        }
    };

    Pipeline::Pass* Pipeline::addWorkPass(const std::string& name, osg::Program* program, GLbitfield clearMask, bool fixedSize, osg::Vec2 sizeScale)
    {
        osg::Camera* camera = new osg::Camera;
        camera->setName(name);
        camera->setGraphicsContext(_graphicsContext);
        camera->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
        camera->setClearMask(clearMask);
        camera->setRenderOrder(osg::Camera::PRE_RENDER);
        camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        camera->setImplicitBufferAttachmentMask(0, 0);
        camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        camera->setViewMatrix(osg::Matrixd::identity());
        camera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
        camera->setCullingMode(osg::CullSettings::NO_CULLING);
        if (fixedSize)
        {
            camera->setViewport(0, 0, sizeScale.x(), sizeScale.y());
        }
        else
        {
            osg::Viewport* nativeViewport = _view->getCamera()->getViewport();
            camera->setViewport(nativeViewport->x(), nativeViewport->y(), nativeViewport->width() * sizeScale.x(), nativeViewport->height() * sizeScale.y());
        }
        osg::Geode* geode = new osg::Geode;
        geode->addDrawable(getScreenGeometry());
        for (uint32_t i = 0; i < program->getNumShaders(); i++)
            program->getShader(i)->setName(name);
        geode->getOrCreateStateSet()->setAttribute(program, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        camera->addChild(geode);

        _view->addSlave(camera, false);
        Pass* newPass = new Pass(camera, fixedSize, sizeScale);
        _passes.push_back(newPass);
        return newPass;
    }

    Pipeline::Pass* Pipeline::addDisplayPass(const std::string& name, osg::Program* program)
    {
        osg::Camera* camera = new osg::Camera;
        camera->setName(name);
        camera->setGraphicsContext(_graphicsContext);
        camera->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
        camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        camera->setRenderOrder(osg::Camera::POST_RENDER);
        camera->setViewport(_view->getCamera()->getViewport());
        camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        camera->setViewMatrix(osg::Matrixd::identity());
        camera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
        camera->setAllowEventFocus(false);
        osg::Geode* geode = new osg::Geode;
        geode->addDrawable(getScreenGeometry());
        for (uint32_t i = 0; i < program->getNumShaders(); i++)
            program->getShader(i)->setName(name);
        geode->getOrCreateStateSet()->setAttribute(program, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        camera->addChild(geode);
        _view->addSlave(camera, false);
        Pass* newPass = new Pass(camera, false, osg::Vec2(1.0, 1.0));
        _passes.push_back(newPass);
        return newPass;
    }
}
