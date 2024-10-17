#pragma once
#include <osgViewer/View>
#include <osg/Texture2D>
#include <osg/Texture2DMultisample>
#include <osg/Geometry>
#include <osg/Geode>

#include <cassert>

namespace xxx
{
    class Pipeline : public osg::Referenced
    {
    public:
        Pipeline(osgViewer::View* view, osg::GraphicsContext* graphicsContext) : mView(view), mGraphicsContext(graphicsContext)
        {
            mView->getCamera()->setGraphicsContext(nullptr);
        }
        virtual ~Pipeline() = default;

        class Pass : public osg::Referenced
        {
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
                osg::Texture::WrapMode wrapS = osg::Texture::CLAMP_TO_EDGE, osg::Texture::WrapMode wrapT = osg::Texture::CLAMP_TO_EDGE);

            void detach(BufferType buffer)
            {
                mCamera->detach(buffer);
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

            const std::string& getName() const
            {
                return mCamera->getName();
            }

            osg::Camera* getCamera() const
            {
                return mCamera;
            }

            osg::Vec2 getSizeScale() const
            {
                return mSizeScale;
            }

            void setEnable(bool enable)
            {
                mCamera->setNodeMask(enable ? 0xFFFFFFFF : 0);
            }

            bool getEnable() const
            {
                return mCamera->getNodeMask() != 0;
            }

            void resize(int width, int height)
            {
                if (mFixedSize)
                    return;

                // get real size
                int realWidth = width * mSizeScale.x();
                int realHeight = height * mSizeScale.y();

                // resize viewport
                mCamera->setViewport(0, 0, realWidth, realHeight);

                // resize attachment texture
                auto& bufferAttachmentMap = mCamera->getBufferAttachmentMap();
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
                mCamera->dirtyAttachmentMap();
                mResolutionUniform->set(osg::Vec4(realWidth, realHeight, 1.0f / realWidth, 1.0f / realHeight));
            }

            bool isDisplayPass() const
            {
                return mCamera->getRenderTargetImplementation() != osg::Camera::FRAME_BUFFER_OBJECT;
            }

            osg::Texture* generateTexture(GLenum internalFormat, bool multisampling,
                osg::Texture::FilterMode minFilter = osg::Texture::LINEAR, osg::Texture::FilterMode magFilter = osg::Texture::LINEAR,
                osg::Texture::WrapMode wrapS = osg::Texture::CLAMP_TO_EDGE, osg::Texture::WrapMode wrapT = osg::Texture::CLAMP_TO_EDGE);

        private:
            osg::ref_ptr<osg::Camera> mCamera;
            bool mFixedSize;
            osg::Vec2 mSizeScale;
            osg::ref_ptr<osg::Uniform> mResolutionUniform;
        };

        osgViewer::View* getView() const
        {
            return mView;
        }

        uint32_t getPassCount() const
        {
            return mPasses.size();
        }

        Pass* getPass(uint32_t index) const
        {
            if (index >= mPasses.size())
                return nullptr;
            return mPasses[index];
        }

        Pass* getPass(const std::string& name) const
        {
            for (Pass* pass : mPasses)
                if (pass->getName() == name)
                    return pass;
            return nullptr;
        }

        uint32_t getPassIndex(Pass* pass) const
        {
            for (uint32_t i = 0; i < mPasses.size(); ++i)
                if (mPasses[i] == pass)
                    return i;
            return uint32_t(-1);
        }

        uint32_t getPassIndex(const std::string& name) const
        {
            for (uint32_t i = 0; i < mPasses.size(); ++i)
                if (mPasses[i]->getName() == name)
                    return i;
            return uint32_t(-1);
        }

        // fixedSize: 是否为固定大小
        // sizeScale: 当fixedSize为true时为默认FBO viewport大小的缩放比例, 否则为固定的viewport大小
        Pass* addInputPass(const std::string& name, osg::Node::NodeMask cullMask, GLbitfield clearMask, bool fixedSize = false, osg::Vec2 sizeScale = osg::Vec2(1.0, 1.0));

        Pass* addWorkPass(const std::string& name, osg::Program* program, GLbitfield clearMask, bool fixedSize = false, osg::Vec2 sizeScale = osg::Vec2(1.0, 1.0));

        Pass* addDisplayPass(const std::string& name, osg::Program* program);

        Pass* insertInputPass(uint32_t pos, const std::string& name, osg::Node::NodeMask cullMask, GLbitfield clearMask, bool fixedSize = false, osg::Vec2 sizeScale = osg::Vec2(1.0, 1.0));

        Pass* insertWorkPass(uint32_t pos, const std::string& name, osg::Program* program, GLbitfield clearMask, bool fixedSize = false, osg::Vec2 sizeScale = osg::Vec2(1.0, 1.0));

        bool removePass(Pass* pass)
        {
            uint32_t slaveIndex = mView->findSlaveIndexForCamera(pass->getCamera());
            if (slaveIndex == mView->getNumSlaves())
                return false;

            mView->removeSlave(slaveIndex);
            mPasses.erase(mPasses.begin() + slaveIndex);
            return true;
        }

        // TODO: process resize
        uint32_t addBlitFramebufferCommand(Pass* srcPass, Pass* dstPass, GLbitfield mask, GLenum filter, osg::Vec4d srcRect = osg::Vec4d(0, 0, 1, 1), osg::Vec4d dstRect = osg::Vec4d(0, 0, 1, 1))
        {
            class BlitFramebufferCommandCallback : public osg::Camera::DrawCallback
            {
            public:
                BlitFramebufferCommandCallback(Pass* srcPass, osg::Vec4d srcRect, Pass* dstPass, osg::Vec4d dstRect, GLbitfield mask, GLenum filter) :
                    mReadFBO(new osg::FrameBufferObject),
                    mDrawFBO(new osg::FrameBufferObject),
                    mSrcPass(srcPass),
                    mDstPass(dstPass),
                    mSrcRect(srcRect),
                    mDstRect(dstRect),
                    mMask(mask),
                    mFilter(filter)
                {
                    using BufferType = osg::Camera::BufferComponent;
                    for (const auto& bufferAttachemnt : mSrcPass->getCamera()->getBufferAttachmentMap())
                    {
                        osg::Texture* texture = bufferAttachemnt.second._texture.get();
                        switch (texture->getTextureTarget())
                        {
                        case GL_TEXTURE_2D:
                            mReadFBO->setAttachment(bufferAttachemnt.first, osg::FrameBufferAttachment(dynamic_cast<osg::Texture2D*>(texture)));
                            break;
                        case GL_TEXTURE_2D_MULTISAMPLE:
                            mReadFBO->setAttachment(bufferAttachemnt.first, osg::FrameBufferAttachment(dynamic_cast<osg::Texture2DMultisample*>(texture)));
                            break;
                        default:
                            break;
                        }
                    }

                    for (const auto& bufferAttachemnt : mDstPass->getCamera()->getBufferAttachmentMap())
                    {
                        osg::Texture* texture = bufferAttachemnt.second._texture.get();
                        switch (texture->getTextureTarget())
                        {
                        case GL_TEXTURE_2D:
                            mDrawFBO->setAttachment(bufferAttachemnt.first, osg::FrameBufferAttachment(dynamic_cast<osg::Texture2D*>(texture)));
                            break;
                        case GL_TEXTURE_2D_MULTISAMPLE:
                            mDrawFBO->setAttachment(bufferAttachemnt.first, osg::FrameBufferAttachment(dynamic_cast<osg::Texture2DMultisample*>(texture)));
                            break;
                        default:
                            break;
                        }
                    }

                    osg::Viewport* srcViewport = mSrcPass->getCamera()->getViewport();
                    mSrcWidth = srcViewport->width();
                    mSrcHeight = srcViewport->height();

                    osg::Viewport* dstViewport = mDstPass->getCamera()->getViewport();
                    mDstWidth = dstViewport->width();
                    mDstHeight = dstViewport->height();
                }

                virtual void operator () (osg::RenderInfo& renderInfo) const
                {
                    osg::Viewport* srcViewport = mSrcPass->getCamera()->getViewport();
                    if (mSrcWidth != srcViewport->width() || mSrcHeight != srcViewport->height())
                    {
                        const_cast<GLuint&>(mSrcWidth) = srcViewport->width();
                        const_cast<GLuint&>(mSrcHeight) = srcViewport->height();

                        for (const auto& bufferAttachemnt : mSrcPass->getCamera()->getBufferAttachmentMap())
                        {
                            osg::Texture* texture = bufferAttachemnt.second._texture.get();
                            switch (texture->getTextureTarget())
                            {
                            case GL_TEXTURE_2D:
                                mReadFBO->setAttachment(bufferAttachemnt.first, osg::FrameBufferAttachment(dynamic_cast<osg::Texture2D*>(texture)));
                                break;
                            case GL_TEXTURE_2D_MULTISAMPLE:
                                mReadFBO->setAttachment(bufferAttachemnt.first, osg::FrameBufferAttachment(dynamic_cast<osg::Texture2DMultisample*>(texture)));
                                break;
                            default:
                                break;
                            }
                        }
                    }
                    GLuint srcX0 = mSrcWidth * mSrcRect.x();
                    GLuint srcY0 = mSrcHeight * mSrcRect.y();
                    GLuint srcX1 = mSrcWidth * mSrcRect.z();
                    GLuint srcY1 = mSrcHeight * mSrcRect.w();

                    osg::Viewport* dstViewport = mDstPass->getCamera()->getViewport();
                    if (mDstWidth != dstViewport->width() || mDstHeight != dstViewport->height())
                    {
                        const_cast<GLuint&>(mDstWidth) = dstViewport->width();
                        const_cast<GLuint&>(mDstHeight) = dstViewport->height();

                        for (const auto& bufferAttachemnt : mDstPass->getCamera()->getBufferAttachmentMap())
                        {
                            osg::Texture* texture = bufferAttachemnt.second._texture.get();
                            switch (texture->getTextureTarget())
                            {
                            case GL_TEXTURE_2D:
                                mDrawFBO->setAttachment(bufferAttachemnt.first, osg::FrameBufferAttachment(dynamic_cast<osg::Texture2D*>(texture)));
                                break;
                            case GL_TEXTURE_2D_MULTISAMPLE:
                                mDrawFBO->setAttachment(bufferAttachemnt.first, osg::FrameBufferAttachment(dynamic_cast<osg::Texture2DMultisample*>(texture)));
                                break;
                            default:
                                break;
                            }
                        }
                    }
                    GLuint dstX0 = mDstWidth * mDstRect.x();
                    GLuint dstY0 = mDstHeight * mDstRect.y();
                    GLuint dstX1 = mDstWidth * mDstRect.z();
                    GLuint dstY1 = mDstHeight * mDstRect.w();

                    mReadFBO->apply(*renderInfo.getState(), osg::FrameBufferObject::READ_FRAMEBUFFER);
                    mDrawFBO->apply(*renderInfo.getState(), osg::FrameBufferObject::DRAW_FRAMEBUFFER);
                    osg::GLExtensions* ext = renderInfo.getState()->get<osg::GLExtensions>();
                    ext->glBlitFramebuffer(
                        srcX0, srcY0, srcX1, srcY1,
                        dstX0, dstY0, dstX1, dstY1,
                        mMask, mFilter
                    );
                }
            protected:
                osg::ref_ptr<osg::FrameBufferObject> mReadFBO;
                osg::ref_ptr<osg::FrameBufferObject> mDrawFBO;
                Pass* mSrcPass;
                Pass* mDstPass;
                osg::Vec4d mDstRect;
                osg::Vec4d mSrcRect;
                GLbitfield mMask;
                GLenum mFilter;
                GLuint mSrcWidth, mSrcHeight;
                GLuint mDstWidth, mDstHeight;
            };

            osg::Camera::DrawCallback* command = new BlitFramebufferCommandCallback(srcPass, srcRect, dstPass, dstRect, mask, filter);
            srcPass->getCamera()->addFinalDrawCallback(command);
            static uint32_t blitFramebufferCommandId = 0;
            mBlitFramebufferCommandMap[blitFramebufferCommandId++] = std::make_pair(srcPass, command);
            return blitFramebufferCommandId - 1;
        }

        bool removeBlitFramebufferCommand(uint32_t id)
        {
            auto findResult = mBlitFramebufferCommandMap.find(id);
            if (findResult != mBlitFramebufferCommandMap.end())
            {
                findResult->second.first->getCamera()->removeFinalDrawCallback(findResult->second.second);
                mBlitFramebufferCommandMap.erase(findResult);
                return true;
            }
            return false;
        }

        void resize(int width, int height, bool resizeInputAndWorkPass, bool resizeDisplayPass)
        {
            // set new aspect
            double fovy, aspect, zNear, zFar;
            mView->getCamera()->getProjectionMatrixAsPerspective(fovy, aspect, zNear, zFar);
            mView->getCamera()->setProjectionMatrixAsPerspective(fovy, double(width) / double(height), zNear, zFar);

            // resize per pass
            for (Pass* pass : mPasses)
            {
                bool isDisplayPass = pass->isDisplayPass();
                if ((!isDisplayPass && resizeInputAndWorkPass) || (isDisplayPass && resizeDisplayPass))
                {
                    pass->resize(width, height);
                }
            }
        }

    private:
        osg::ref_ptr<osgViewer::View> mView;
        osg::ref_ptr<osg::GraphicsContext> mGraphicsContext;
        std::vector<osg::ref_ptr<Pass>> mPasses;
        std::unordered_map<uint32_t, std::pair<Pass*, osg::Camera::DrawCallback*>> mBlitFramebufferCommandMap;

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
    };
}
