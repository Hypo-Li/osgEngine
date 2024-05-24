#pragma once
#include <osgViewer/Viewer>
// Impl
#include <osg/Texture2D>
#include <cassert>

namespace xxx
{
    class Pipeline : public osg::Referenced
    {
    public:
        Pipeline(osgViewer::Viewer* viewer)
        {
            _viewer = viewer;
            osg::Camera* nativeCamera = viewer->getCamera();
            _graphicsContext = nativeCamera->getGraphicsContext();
            osg::Viewport* nativeViewport = nativeCamera->getViewport();
            _graphicsContext->setResizedCallback(new ResizedCallback(this, nativeViewport->width(), nativeViewport->height()));
            nativeCamera->setGraphicsContext(nullptr);
        }
        virtual ~Pipeline() = default;

        class ResizedCallback;
        class Pass : public osg::Referenced
        {
            friend class Pipeline;
            friend class Pipeline::ResizedCallback;
        public:
            Pass(osg::Camera* camera, bool fixedSize, osg::Vec2d sizeScale) : _camera(camera), _fixedSize(fixedSize), _sizeScale(sizeScale)
            {
                osg::Viewport* viewport = _camera->getViewport();
                _resolutionUniform = new osg::Uniform("uResolution", osg::Vec4(viewport->width(), viewport->height(), 1.0f / viewport->width(), 1.0f / viewport->height()));
                _camera->getOrCreateStateSet()->addUniform(_resolutionUniform);
            }

            using BufferType = osg::Camera::BufferComponent;

            void attach(BufferType buffer, osg::Texture* texture, int level = 0, int face = 0, bool mipmapGeneration = false)
            {
                _camera->attach(buffer, texture, level, face, mipmapGeneration);
            }

            void attach(BufferType buffer, GLenum internalFormat,
                osg::Texture::FilterMode minFilter = osg::Texture::LINEAR, osg::Texture::FilterMode magFilter = osg::Texture::LINEAR,
                osg::Texture::WrapMode wrapS = osg::Texture::CLAMP_TO_EDGE, osg::Texture::WrapMode wrapT = osg::Texture::CLAMP_TO_EDGE)
            {
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
                osg::Viewport* viewport = _camera->getViewport();
                osg::Texture2D* texture = new osg::Texture2D;
                texture->setTextureSize(viewport->width(), viewport->height());
                texture->setInternalFormat(internalFormat);
                constexpr uint32_t count = sizeof(formatTable) / sizeof(std::pair<GLenum, GLenum>);
                for (uint32_t i = 0; i < count; i++)
                {
                    if (formatTable[i].first == internalFormat)
                        texture->setSourceFormat(formatTable[i].second);
                }
                //texture->setSourceType(sourceType);
                texture->setFilter(osg::Texture::MIN_FILTER, minFilter);
                texture->setFilter(osg::Texture::MAG_FILTER, magFilter);
                texture->setWrap(osg::Texture::WRAP_S, wrapS);
                texture->setWrap(osg::Texture::WRAP_T, wrapT);
                _camera->attach(buffer, texture);
            }

            osg::Texture* getBufferTexture(BufferType buffer)
            {
                return _camera->getBufferAttachmentMap().at(buffer)._texture;
            }

            void applyUniform(osg::Uniform* uniform)
            {
                _camera->getOrCreateStateSet()->addUniform(uniform, osg::StateAttribute::ON);
            }

            void applyTexture(osg::Texture* texture, const char* name, int unit)
            {
                _camera->getOrCreateStateSet()->addUniform(new osg::Uniform(name, unit));
                _camera->getOrCreateStateSet()->setTextureAttribute(unit, texture, osg::StateAttribute::ON);
            }

            void setAttribute(osg::StateAttribute* attribute, osg::StateAttribute::OverrideValue value = osg::StateAttribute::ON)
            {
                _camera->getOrCreateStateSet()->setAttribute(attribute, value);
            }

            void setMode(osg::StateAttribute::GLMode mode, osg::StateAttribute::GLModeValue value = osg::StateAttribute::ON)
            {
                _camera->getOrCreateStateSet()->setMode(mode, value);
            }

            osg::Camera* getCamera() const
            {
                return _camera;
            }

        private:
            osg::ref_ptr<osg::Camera> _camera;
            bool _fixedSize;
            osg::Vec2d _sizeScale;
            osg::ref_ptr<osg::Uniform> _resolutionUniform;
        };

        Pass* addInputPass(const char* name, osg::Node::NodeMask cullMask, GLbitfield clearMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, bool fixedSize = false, osg::Vec2d sizeScale = osg::Vec2d(1.0, 1.0))
        {
            osg::Camera* camera = new osg::Camera;
            camera->setName(name);
            camera->setGraphicsContext(_graphicsContext);
            camera->setCullMask(cullMask);
            camera->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
            camera->setClearMask(clearMask);
            camera->setRenderOrder(osg::Camera::PRE_RENDER);
            camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
            if (fixedSize)
            {
                camera->setViewport(0, 0, sizeScale.x(), sizeScale.y());
                camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
            }
            else
            {
                osg::Viewport* nativeViewport = _viewer->getCamera()->getViewport();
                int x = nativeViewport->x();
                int y = nativeViewport->y();
                int width = nativeViewport->width();
                int height = nativeViewport->height();
                camera->setViewport(x, y, width * sizeScale.x(), height * sizeScale.y());
            }
            camera->setImplicitBufferAttachmentMask(0, 0);
            _viewer->addSlave(camera, true);
            Pass* newPass = new Pass(camera, fixedSize, sizeScale);
            _passes.push_back(newPass);
            return newPass;
        }

        Pass* addWorkPass(const char* name, osg::Program* program, GLbitfield clearMask = GL_COLOR_BUFFER_BIT, bool fixedSize = false, osg::Vec2d sizeScale = osg::Vec2d(1.0, 1.0))
        {
            osg::Camera* camera = new osg::Camera;
            camera->setName(name);
            camera->setGraphicsContext(_graphicsContext);
            camera->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
            camera->setClearMask(clearMask);
            camera->setRenderOrder(osg::Camera::PRE_RENDER);
            camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
            if (fixedSize)
            {
                camera->setViewport(0, 0, sizeScale.x(), sizeScale.y());
            }
            else
            {
                osg::Viewport* nativeViewport = _viewer->getCamera()->getViewport();
                int x = nativeViewport->x();
                int y = nativeViewport->y();
                int width = nativeViewport->width();
                int height = nativeViewport->height();
                camera->setViewport(x, y, width * sizeScale.x(), height * sizeScale.y());
            }
            camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
            camera->setViewMatrix(osg::Matrixd::identity());
            camera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
            camera->setImplicitBufferAttachmentMask(0, 0);
            camera->setCullingMode(osg::CullSettings::NO_CULLING);
            osg::Geode* geode = new osg::Geode;
            geode->addDrawable(getScreenGeometry());
            for (uint32_t i = 0; i < program->getNumShaders(); i++)
                program->getShader(i)->setName(name);
            geode->getOrCreateStateSet()->setAttribute(program, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            camera->addChild(geode);
            _viewer->addSlave(camera, false);
            Pass* newPass = new Pass(camera, fixedSize, sizeScale);
            _passes.push_back(newPass);
            return newPass;
        }

        Pass* addFinalPass(const char* name, osg::Program* program)
        {
            osg::Camera* camera = new osg::Camera;
            camera->setName(name);
            camera->setGraphicsContext(_graphicsContext);
            camera->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
            camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            camera->setRenderOrder(osg::Camera::POST_RENDER);
            camera->setViewport(_viewer->getCamera()->getViewport());
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
            _viewer->addSlave(camera, false);
            Pass* newPass = new Pass(camera, false, osg::Vec2d(1.0, 1.0));
            _passes.push_back(newPass);
            return newPass;
        }

        void resize(int width, int height, bool resizeFinalPass)
        {
            double fovy, aspect, zNear, zFar;
            _viewer->getCamera()->getProjectionMatrixAsPerspective(fovy, aspect, zNear, zFar);
            _viewer->getCamera()->setProjectionMatrixAsPerspective(fovy, double(width) / double(height), zNear, zFar);
            for (Pass* pass : _passes)
            {
                bool isFinalPass = pass->_camera->getRenderTargetImplementation() != osg::Camera::FRAME_BUFFER_OBJECT;
                if ((!isFinalPass || (isFinalPass && resizeFinalPass)) && !pass->_fixedSize)
                {
                    // resize viewport
                    int realWidth = width * pass->_sizeScale.x();
                    int realHeight = height * pass->_sizeScale.y();
                    pass->_camera->setViewport(0, 0, realWidth, realHeight);

                    // resize fbo texture
                    auto& bufferAttachmentMap = pass->_camera->getBufferAttachmentMap();
                    for (auto itr : bufferAttachmentMap)
                    {
                        osg::Texture2D* texture = dynamic_cast<osg::Texture2D*>(itr.second._texture.get());
                        assert(texture && "Other type texture resize not implementation");
                        texture->setTextureSize(realWidth, realHeight);
                        texture->dirtyTextureObject();
                    }
                    pass->_camera->dirtyAttachmentMap();
                    pass->_resolutionUniform->set(osg::Vec4(realWidth, realHeight, 1.0f / realWidth, 1.0f / realHeight));
                }
            }
        }

        osg::Texture2D* createScreenTexture(GLenum internalFormat, osg::Vec2d sizeScale = osg::Vec2d(1.0, 1.0), 
                osg::Texture::FilterMode minFilter = osg::Texture::LINEAR, osg::Texture::FilterMode magFilter = osg::Texture::LINEAR,
                osg::Texture::WrapMode wrapS = osg::Texture::CLAMP_TO_EDGE, osg::Texture::WrapMode wrapT = osg::Texture::CLAMP_TO_EDGE)
        {
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
            osg::Viewport* viewport = _viewer->getCamera()->getViewport();
            osg::Texture2D* texture = new osg::Texture2D;
            texture->setTextureSize(viewport->width(), viewport->height());
            texture->setInternalFormat(internalFormat);
            constexpr uint32_t count = sizeof(formatTable) / sizeof(std::pair<GLenum, GLenum>);
            for (uint32_t i = 0; i < count; i++)
            {
                if (formatTable[i].first == internalFormat)
                    texture->setSourceFormat(formatTable[i].second);
            }
            //texture->setSourceType(sourceType);
            texture->setFilter(osg::Texture::MIN_FILTER, minFilter);
            texture->setFilter(osg::Texture::MAG_FILTER, magFilter);
            texture->setWrap(osg::Texture::WRAP_S, wrapS);
            texture->setWrap(osg::Texture::WRAP_T, wrapT);
            return texture;
        }

    private:
        osg::ref_ptr<osgViewer::Viewer> _viewer;
        osg::ref_ptr<osg::GraphicsContext> _graphicsContext;
        std::vector<osg::ref_ptr<Pass>> _passes;

        static osg::Geometry* Pipeline::getScreenGeometry()
        {
            static osg::ref_ptr<osg::Geometry> geometry = nullptr;
            if (geometry) return geometry;
            geometry = new osg::Geometry;
            geometry->setUseDisplayList(false);
            geometry->setUseVertexBufferObjects(true);
            osg::Vec3Array* positions = new osg::Vec3Array();
            positions->push_back({ -1.f, 3.f, 0.f });
            positions->push_back({ -1.f, -1.f, 0.f });
            positions->push_back({ 3.f, -1.f, 0.f });
            geometry->setVertexAttribArray(0, positions);
            geometry->setVertexAttribBinding(0, osg::Geometry::BIND_PER_VERTEX);
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, 3));
            geometry->setCullingActive(false);
            return geometry;
        }

        class ResizedCallback : public osg::GraphicsContext::ResizedCallback
        {
            osg::ref_ptr<xxx::Pipeline> _pipeline;
            int _width, _height;
        public:
            ResizedCallback(xxx::Pipeline* pipeline, int width, int height) : _pipeline(pipeline), _width(width), _height(height) {}

            virtual void resizedImplementation(osg::GraphicsContext* gc, int x, int y, int width, int height)
            {
                if ((width == _width && height == _height) || (width == 1 && height == 1))
                    return;
                _width = width, _height = height;
                _pipeline->resize(width, height, true);
            }
        };
    };
}
