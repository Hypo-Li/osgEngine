#pragma once
#include <osg/View>
// Impl
#include <osg/Texture2D>
#include <osg/Texture2DMultisample>
#include <osg/Geometry>
#include <osg/Geode>
#include <cassert>

namespace xxx
{
    class MyTexture2DMultisample : public osg::Texture2DMultisample
    {
    public:
        MyTexture2DMultisample();

        MyTexture2DMultisample(GLsizei numSamples, GLboolean fixedsamplelocations);

        virtual void apply(osg::State& state) const override;
    };

    class Pipeline : public osg::Referenced
    {
        class ResizedCallback;
    public:
        Pipeline(osg::View* view, osg::GraphicsContext* graphicsContext) : mView(view), mGraphicsContext(graphicsContext)
        {
            mGraphicsContext->setResizedCallback(new ResizedCallback(this));
            mView->getCamera()->setGraphicsContext(nullptr);
        }
        virtual ~Pipeline() = default;

        class Pass : public osg::Referenced
        {
            friend class Pipeline;
            friend class Pipeline::ResizedCallback;
        public:
            Pass(osg::Camera* camera, bool fixedSize, osg::Vec2 sizeScale) : mCamera(camera), mFixedSize(fixedSize), mSizeScale(sizeScale)
            {
                osg::Viewport* viewport = mCamera->getViewport();
                mResolutionUniform = new osg::Uniform("uResolution", osg::Vec4(viewport->width(), viewport->height(), 1.0f / viewport->width(), 1.0f / viewport->height()));
                mCamera->getOrCreateStateSet()->addUniform(mResolutionUniform);
            }

            using BufferType = osg::Camera::BufferComponent;

            void attach(BufferType buffer, osg::Texture* texture, int level = 0, int face = 0, bool mipmapGeneration = false)
            {
                mCamera->attach(buffer, texture, level, face, mipmapGeneration);
            }

            void attach(BufferType buffer, GLenum internalFormat, bool multisampling = false,
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
                osg::Viewport* viewport = mCamera->getViewport();
                osg::Texture* texture;
                if (multisampling)
                {
                    osg::Texture2DMultisample* tex2dms = new MyTexture2DMultisample(4, true);
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
                constexpr uint32_t count = sizeof(formatTable) / sizeof(std::pair<GLenum, GLenum>);
                for (uint32_t i = 0; i < count; i++)
                {
                    if (formatTable[i].first == internalFormat)
                        texture->setSourceFormat(formatTable[i].second);
                }
                texture->setFilter(osg::Texture::MIN_FILTER, minFilter);
                texture->setFilter(osg::Texture::MAG_FILTER, magFilter);
                texture->setWrap(osg::Texture::WRAP_S, wrapS);
                texture->setWrap(osg::Texture::WRAP_T, wrapT);
                mCamera->attach(buffer, texture);
            }

            osg::Texture* getBufferTexture(BufferType buffer)
            {
                return mCamera->getBufferAttachmentMap().at(buffer)._texture.get();
            }

            void applyUniform(osg::Uniform* uniform)
            {
                mCamera->getOrCreateStateSet()->addUniform(uniform, osg::StateAttribute::ON);
            }

            void applyTexture(osg::Texture* texture, const std::string& name, int unit)
            {
                mCamera->getOrCreateStateSet()->addUniform(new osg::Uniform(name.c_str(), unit));
                mCamera->getOrCreateStateSet()->setTextureAttribute(unit, texture, osg::StateAttribute::ON);
            }

            void setAttribute(osg::StateAttribute* attribute, osg::StateAttribute::OverrideValue value = osg::StateAttribute::ON)
            {
                mCamera->getOrCreateStateSet()->setAttribute(attribute, value);
            }

            void setMode(osg::StateAttribute::GLMode mode, osg::StateAttribute::GLModeValue value = osg::StateAttribute::ON)
            {
                mCamera->getOrCreateStateSet()->setMode(mode, value);
            }

            osg::Camera* getCamera() const
            {
                return mCamera;
            }

            osg::Vec2 getSizeScale() const
            {
                return mSizeScale;
            }

        private:
            osg::ref_ptr<osg::Camera> mCamera;
            bool mFixedSize;
            osg::Vec2 mSizeScale;
            osg::ref_ptr<osg::Uniform> mResolutionUniform;
        };

        osg::View* getView() const
        {
            return mView;
        }

        const std::vector<osg::ref_ptr<Pass>> getPasses() const
        {
            return mPasses;
        }

        osg::GraphicsContext* getOsgGraphicsContext() const
        {
            return mGraphicsContext;
        }

        // fixedSize: 是否为固定大小
        // sizeScale: 当fixedSize为true时为相较于默认FBO viewport大小的缩放比例, 否则为固定的viewport大小
        Pass* addInputPass(const std::string& name, osg::Node::NodeMask cullMask, GLbitfield clearMask, bool fixedSize = false, osg::Vec2 sizeScale = osg::Vec2(1.0, 1.0));

        Pass* addWorkPass(const std::string& name, osg::Program* program, GLbitfield clearMask, bool fixedSize = false, osg::Vec2 sizeScale = osg::Vec2(1.0, 1.0));

        Pass* addDisplayPass(const std::string& name, osg::Program* program);

        void resize(int width, int height, bool resizeDisplayPass)
        {
            // set new aspect
            double fovy, aspect, zNear, zFar;
            mView->getCamera()->getProjectionMatrixAsPerspective(fovy, aspect, zNear, zFar);
            mView->getCamera()->setProjectionMatrixAsPerspective(fovy, double(width) / double(height), zNear, zFar);
            // resize passes expect final pass
            // final pass will resize by osg automatically
            for (Pass* pass : mPasses)
            {
                bool isDisplayPass = pass->mCamera->getRenderTargetImplementation() != osg::Camera::FRAME_BUFFER_OBJECT;
                if (!pass->mFixedSize && ( !isDisplayPass || (isDisplayPass && resizeDisplayPass) ) )
                {
                    // get real size
                    int realWidth = width * pass->mSizeScale.x();
                    int realHeight = height * pass->mSizeScale.y();

                    // resize viewport
                    pass->mCamera->setViewport(0, 0, realWidth, realHeight);

                    // resize attachment texture
                    auto& bufferAttachmentMap = pass->mCamera->getBufferAttachmentMap();
                    for (auto itr : bufferAttachmentMap)
                    {
                        osg::Texture2D* texture2d = dynamic_cast<osg::Texture2D*>(itr.second._texture.get());
                        osg::Texture2DMultisample* texture2dMs = dynamic_cast<osg::Texture2DMultisample*>(itr.second._texture.get());
                        assert((texture2d || texture2dMs) && "Unsupport texture type!");
                        if (texture2d)
                        {
                            texture2d->setTextureSize(realWidth, realHeight);
                            texture2d->dirtyTextureObject();
                        }
                        else if (texture2dMs)
                        {
                            texture2dMs->setTextureSize(realWidth, realHeight);
                            texture2dMs->dirtyTextureObject();
                        }
                    }
                    pass->mCamera->dirtyAttachmentMap();
                    pass->mResolutionUniform->set(osg::Vec4(realWidth, realHeight, 1.0f / realWidth, 1.0f / realHeight));
                }
            }
        }

        osg::Texture2D* createScreenTexture(GLenum internalFormat, osg::Vec2 sizeScale = osg::Vec2(1.0, 1.0), 
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
            osg::Viewport* viewport = mView->getCamera()->getViewport();
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
        osg::ref_ptr<osg::View> mView;
        osg::ref_ptr<osg::GraphicsContext> mGraphicsContext;
        std::vector<osg::ref_ptr<Pass>> mPasses;

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
            osg::ref_ptr<xxx::Pipeline> mPipeline;
            int mWidth, mHeight;
        public:
            ResizedCallback(xxx::Pipeline* pipeline) : mPipeline(pipeline)
            {
                osg::Viewport* viewport = pipeline->getView()->getCamera()->getViewport();
                mWidth = viewport->width();
                mHeight = viewport->height();
            }

            virtual void resizedImplementation(osg::GraphicsContext* gc, int x, int y, int width, int height)
            {
                if ((width == mWidth && height == mHeight) || (width == 1 && height == 1))
                    return;
                mWidth = width, mHeight = height;
                mPipeline->resize(width, height, true);
            }
        };
    };
}
