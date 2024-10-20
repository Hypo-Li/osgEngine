#include "Pipeline.h"

namespace xxx
{

    class Texture2DMultisampleFixed : public osg::Texture2DMultisample
    {
    public:
        Texture2DMultisampleFixed(GLsizei numSamples, GLboolean fixedsamplelocations) :
            osg::Texture2DMultisample(numSamples, fixedsamplelocations)
        {}

        virtual void apply(osg::State& state) const override
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
    };

    osg::Texture* Pipeline::Pass::generateTexture(GLenum internalFormat, bool multisampling,
                osg::Texture::FilterMode minFilter, osg::Texture::FilterMode magFilter,
                osg::Texture::WrapMode wrapS, osg::Texture::WrapMode wrapT)
    {
        // this special format need set source type
        static constexpr std::pair<GLenum, GLenum> formatTable[] = {
            { GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT },
            { GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT },
            { GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT },
            { GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT },
            { GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT },
            { GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL },
            { GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL },
            { GL_R11F_G11F_B10F, GL_RGB },
            { GL_RGB10_A2, GL_RGBA },
        };
        osg::Viewport* viewport = mCamera->getViewport();
        osg::Texture* texture;
        if (multisampling)
        {
            osg::Texture2DMultisample* tex2dms = new Texture2DMultisampleFixed(4, true);
            texture = tex2dms;
            tex2dms->setTextureSize(viewport->width(), viewport->height());
        }
        else
        {
            osg::Texture2D* tex2d = new osg::Texture2D;
            texture = tex2d;
            tex2d->setTextureSize(viewport->width(), viewport->height());
        }
        texture->setInternalFormat(internalFormat);
      
        constexpr uint32_t formatCount = sizeof(formatTable) / sizeof(std::pair<GLenum, GLenum>);
        for (uint32_t i = 0; i < formatCount; i++)
        {
            if (formatTable[i].first == internalFormat)
                texture->setSourceFormat(formatTable[i].second);
        }
        texture->setFilter(osg::Texture::MIN_FILTER, minFilter);
        texture->setFilter(osg::Texture::MAG_FILTER, magFilter);
        texture->setWrap(osg::Texture::WRAP_S, wrapS);
        texture->setWrap(osg::Texture::WRAP_T, wrapT);
        return texture;
    }

    void Pipeline::Pass::attach(BufferType buffer, GLenum internalFormat, bool multisampling,
        osg::Texture::FilterMode minFilter, osg::Texture::FilterMode magFilter,
        osg::Texture::WrapMode wrapS, osg::Texture::WrapMode wrapT)
    {
        mCamera->attach(buffer, generateTexture(internalFormat, multisampling, minFilter, magFilter, wrapS, wrapT));
    }

    Pipeline::Pass* Pipeline::addInputPass(const std::string& name, osg::Node::NodeMask cullMask, GLbitfield clearMask, bool fixedSize, osg::Vec2 sizeScale)
    {
        osg::Camera* camera = new osg::Camera;
        camera->setName(name);
        camera->setGraphicsContext(mGraphicsContext);
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
            osg::Viewport* nativeViewport = mView->getCamera()->getViewport();
            camera->setViewport(nativeViewport->x(), nativeViewport->y(), nativeViewport->width() * sizeScale.x(), nativeViewport->height() * sizeScale.y());
        }

        mView->addSlave(camera, true);
        Pass* newPass = new Pass(camera, fixedSize, sizeScale);
        mPasses.push_back(newPass);
        return newPass;
    }

    Pipeline::Pass* Pipeline::addWorkPass(const std::string& name, osg::Program* program, GLbitfield clearMask, bool fixedSize, osg::Vec2 sizeScale)
    {
        osg::Camera* camera = new osg::Camera;
        camera->setName(name);
        camera->setGraphicsContext(mGraphicsContext);
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
            osg::Viewport* nativeViewport = mView->getCamera()->getViewport();
            camera->setViewport(nativeViewport->x(), nativeViewport->y(), nativeViewport->width() * sizeScale.x(), nativeViewport->height() * sizeScale.y());
        }
        osg::Geode* geode = new osg::Geode;
        geode->addDrawable(getScreenGeometry());
        for (uint32_t i = 0; i < program->getNumShaders(); i++)
            program->getShader(i)->setName(name);
        geode->getOrCreateStateSet()->setAttribute(program, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        camera->addChild(geode);

        mView->addSlave(camera, false);
        Pass* newPass = new Pass(camera, fixedSize, sizeScale);
        mPasses.push_back(newPass);
        return newPass;
    }

    Pipeline::Pass* Pipeline::addDisplayPass(const std::string& name, osg::Program* program)
    {
        osg::Camera* camera = new osg::Camera;
        camera->setName(name);
        camera->setGraphicsContext(mGraphicsContext);
        camera->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
        camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        camera->setRenderOrder(osg::Camera::POST_RENDER);
        camera->setViewport(mView->getCamera()->getViewport());
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
        mView->addSlave(camera, false);
        Pass* newPass = new Pass(camera, false, osg::Vec2(1.0, 1.0));
        mPasses.push_back(newPass);
        return newPass;
    }

    class ViewExt : public osgViewer::View
    {
    public:
        bool insertSlave(uint32_t pos, osg::Camera* camera, const osg::Matrix& projectionOffset, const osg::Matrix& viewOffset, bool useMastersSceneData = true)
        {
            if (!camera) return false;

            camera->setView(this);

            if (useMastersSceneData)
            {
                camera->removeChildren(0, camera->getNumChildren());

                if (_camera.valid())
                {
                    for (unsigned int i = 0; i < _camera->getNumChildren(); ++i)
                    {
                        camera->addChild(_camera->getChild(i));
                    }
                }
            }

            if (_slaves.size() < pos)
                return false;

            _slaves.insert(_slaves.begin() + pos, Slave(camera, projectionOffset, viewOffset, useMastersSceneData));
            _slaves[pos].updateSlave(*this);

            camera->setRenderer(createRenderer(camera));

            return true;
        }

        bool insertSlave(uint32_t pos, osg::Camera* camera, bool useMastersSceneData = true)
        {
            return insertSlave(pos, camera, osg::Matrix::identity(), osg::Matrix::identity(), useMastersSceneData);
        }
    };

    class GraphicsContextExt : osg::GraphicsContext
    {
    public:
        void insertCamera(int pos, osg::Camera* camera)
        {
            auto it = _cameras.begin();
            std::advance(it, pos);
            _cameras.insert(it, camera);
        }
    };

    class CameraExt : public osg::Camera
    {
    public:
        void setGraphicsContextExt(osg::GraphicsContext* context, int cameraPos)
        {
            if (_graphicsContext == context) return;

            //if (_graphicsContext.valid()) _graphicsContext->removeCamera(this);

            _graphicsContext = context;

            if (_graphicsContext.valid()) reinterpret_cast<GraphicsContextExt*>(_graphicsContext.get())->insertCamera(cameraPos, this);
        }
    };

    Pipeline::Pass* Pipeline::insertInputPass(uint32_t pos, const std::string& name, osg::Node::NodeMask cullMask, GLbitfield clearMask, bool fixedSize, osg::Vec2 sizeScale)
    {
        osg::Camera* camera = new osg::Camera;
        camera->setName(name);
        //camera->setGraphicsContext(mGraphicsContext);
        static_cast<CameraExt*>(camera)->setGraphicsContextExt(mGraphicsContext, pos);
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
            osg::Viewport* nativeViewport = mView->getCamera()->getViewport();
            camera->setViewport(nativeViewport->x(), nativeViewport->y(), nativeViewport->width() * sizeScale.x(), nativeViewport->height() * sizeScale.y());
        }

        if (static_cast<ViewExt*>(mView.get())->insertSlave(pos, camera, true))
        {
            Pass* newPass = new Pass(camera, fixedSize, sizeScale);
            mPasses.insert(mPasses.begin() + pos, newPass);
            return newPass;
        }
        return nullptr;
    }

    Pipeline::Pass* Pipeline::insertWorkPass(uint32_t pos, const std::string& name, osg::Program* program, GLbitfield clearMask, bool fixedSize, osg::Vec2 sizeScale)
    {
        osg::Camera* camera = new osg::Camera;
        camera->setName(name);
        //camera->setGraphicsContext(mGraphicsContext);
        static_cast<CameraExt*>(camera)->setGraphicsContextExt(mGraphicsContext, pos);
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
            osg::Viewport* nativeViewport = mView->getCamera()->getViewport();
            camera->setViewport(nativeViewport->x(), nativeViewport->y(), nativeViewport->width() * sizeScale.x(), nativeViewport->height() * sizeScale.y());
        }
        osg::Geode* geode = new osg::Geode;
        geode->addDrawable(getScreenGeometry());
        for (uint32_t i = 0; i < program->getNumShaders(); i++)
            program->getShader(i)->setName(name);
        geode->getOrCreateStateSet()->setAttribute(program, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        camera->addChild(geode);

        if (static_cast<ViewExt*>(mView.get())->insertSlave(pos, camera, false))
        {
            Pass* newPass = new Pass(camera, fixedSize, sizeScale);
            mPasses.insert(mPasses.begin() + pos, newPass);
            return newPass;
        }
        return nullptr;
    }
}
