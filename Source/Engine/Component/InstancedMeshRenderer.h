#pragma once
#include "MeshRenderer.h"

#include <osg/TextureBuffer>
#include <osg/BufferObject>
#include <osg/BufferIndexBinding>

namespace xxx
{
    class InstancedMeshRenderer : public MeshRenderer
    {
        REFLECT_CLASS(InstancedMeshRenderer)
    public:
        struct InstancedData
        {
            osg::Vec3f translation;
            osg::Vec3f rotation;
            osg::Vec3f scale;
        };

        struct InstancedDrawableComputeBoundingBoxCallback : public osg::Drawable::ComputeBoundingBoxCallback
        {
            const std::vector<InstancedData>& mInstancedDatas;
            mutable osg::BoundingBox mNativeBoundingBox;

            InstancedDrawableComputeBoundingBoxCallback(const std::vector<InstancedData>& instancedDatas) : mInstancedDatas(instancedDatas) {}

            virtual osg::BoundingBox computeBound(const osg::Drawable& drawable) const
            {
                osg::BoundingBox result;
                if (!mNativeBoundingBox.valid())
                    mNativeBoundingBox = drawable.computeBoundingBox();

                for (const auto& instancedData : mInstancedDatas)
                {
                    osg::Quat quat(
                        osg::DegreesToRadians(instancedData.rotation.x()), osg::X_AXIS,
                        osg::DegreesToRadians(instancedData.rotation.y()), osg::Y_AXIS,
                        osg::DegreesToRadians(instancedData.rotation.z()), osg::Z_AXIS
                    );
                    osg::Matrixf relativeMatrix = osg::Matrixf::scale(instancedData.scale) * osg::Matrixf::rotate(quat) * osg::Matrixf::translate(instancedData.translation);

                    for (int i = 0; i < 8; ++i)
                    {
                        osg::Vec3f corner = mNativeBoundingBox.corner(i) * relativeMatrix;
                        result.expandBy(corner);
                    }
                }
               
                return result;
            }
        };

        InstancedMeshRenderer()
        {
            
        }

        virtual Type getType() const override
        {
            return Type::InstancedMeshRenderer;
        }

        virtual void onEnable() override
        {
            createInstancedDatasTBO();
            syncWithMesh();
        }

        virtual void onDisable() override
        {

        }

        virtual void syncWithMesh() override
        {
            MeshRenderer::syncWithMesh();
            if (mOsgLOD->getNumChildren() > 0)
            {
                mNativeBoundingSphere = mOsgLOD->getChild(0)->getBound();
            }

            auto geometryDrawCallback = new InstancedGeometryDrawCallback(mDrawnInstanceCount);

            for (const auto& lodGeodes : mOsgGeodes)
            {
                for (osg::Geode* geode : lodGeodes)
                {
                    osg::Geometry* geometry = dynamic_cast<osg::Geometry*>(geode->getDrawable(0));
                    osg::Geometry* instancedGeometry = new osg::Geometry(*geometry);
                    instancedGeometry->setDrawCallback(geometryDrawCallback);
                    instancedGeometry->setComputeBoundingBoxCallback(new InstancedDrawableComputeBoundingBoxCallback(mInstancedDatas));
                    osg::DrawElements* instancedDrawElements = dynamic_cast<osg::DrawElements*>(instancedGeometry->getPrimitiveSet(0)->clone(osg::CopyOp::SHALLOW_COPY));
                    instancedDrawElements->setNumInstances(mInstancedDatas.size());
                    instancedGeometry->setPrimitiveSet(0, instancedDrawElements);
                    geode->setDrawable(0, instancedGeometry);
                }
            }
            if (mInstancedDatas.empty())
                mOsgLOD->setNodeMask(0);
        }

        void addInstance(osg::Vec3f translation = osg::Vec3f(0, 0, 0), osg::Vec3f rotation = osg::Vec3f(0, 0, 0), osg::Vec3f scale = osg::Vec3f(1, 1, 1))
        {
            if (mInstancedDatas.empty())
                mOsgLOD->setNodeMask(~0);
            mInstancedDatas.emplace_back(InstancedData{ translation, rotation, scale });

            for (const auto& lodGeodes : mOsgGeodes)
            {
                for (osg::Geode* geode : lodGeodes)
                {
                    osg::Geometry* geometry = dynamic_cast<osg::Geometry*>(geode->getDrawable(0));
                    osg::DrawElements* instancedDrawElements = dynamic_cast<osg::DrawElements*>(geometry->getPrimitiveSet(0));
                    instancedDrawElements->setNumInstances(mInstancedDatas.size());

                    geometry->dirtyBound();
                }
            }

            mOsgLOD->dirtyBound();
            updateInstancedDatasTBO();
        }

        void setInstance(size_t index, osg::Vec3f translation = osg::Vec3f(0, 0, 0), osg::Vec3f rotation = osg::Vec3f(0, 0, 0), osg::Vec3f scale = osg::Vec3f(1, 1, 1))
        {
            if (index >= mInstancedDatas.size())
                return;
            mInstancedDatas[index].translation = translation;
            mInstancedDatas[index].rotation = rotation;
            mInstancedDatas[index].scale = scale;

            for (const auto& lodGeodes : mOsgGeodes)
                for (osg::Geode* geode : lodGeodes)
                    geode->getDrawable(0)->dirtyBound();

            mOsgLOD->dirtyBound();
            updateInstancedData(index);
        }

        const std::vector<InstancedData>& getInstancedDatas() const
        {
            return mInstancedDatas;
        }

    protected:
        std::vector<InstancedData> mInstancedDatas;

        osg::ref_ptr<osg::TextureBuffer> mInstancedDatasTBO;
        osg::ref_ptr<osg::TextureBuffer> mInstancedRemapTBO;
        osg::ref_ptr<osg::ShaderStorageBufferObject> mRemapCountSSBO;
        uint32_t mDrawnInstanceCount;
        osg::ref_ptr<osg::DispatchCompute> mCullingDispatch;
        osg::ref_ptr<osg::Uniform> mBoundingSphereRadiusUniform;
        osg::ref_ptr<osg::Uniform> mInstanceCountUniform;
        osg::BoundingSphere mNativeBoundingSphere;

        class BoundingSphereRadiusUniformCallback : public osg::UniformCallback
        {
            const osg::BoundingSphere& mBoundingSphere;
        public:
            BoundingSphereRadiusUniformCallback(const osg::BoundingSphere& boundingSphere) : mBoundingSphere(boundingSphere) {}

            virtual void operator () (osg::Uniform* uniform, osg::NodeVisitor* nv)
            {
                uniform->set(float(mBoundingSphere.radius()));
            }
        };

        class InstanceCountUniformCallback : public osg::UniformCallback
        {
            const std::vector<InstancedData>& mInstancedDatas;
        public:
            InstanceCountUniformCallback(const std::vector<InstancedData>& instancedDatas) : mInstancedDatas(instancedDatas) {}

            virtual void operator () (osg::Uniform* uniform, osg::NodeVisitor* nv)
            {
                uniform->set(uint32_t(mInstancedDatas.size()));
            }
        };

        class CullingDispatchDrawCallback : public osg::Drawable::DrawCallback
        {
            osg::ref_ptr<osg::ShaderStorageBufferObject> mRemapCountSSBO;
            uint32_t& mDrawnInstanceCount;
        public:
            CullingDispatchDrawCallback(osg::ShaderStorageBufferObject* remapCountSSBO, uint32_t& drawnInstanceCount) : mRemapCountSSBO(remapCountSSBO), mDrawnInstanceCount(drawnInstanceCount) {}

            virtual void drawImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* drawable) const
            {
                drawable->drawImplementation(renderInfo);
                osg::State* state = renderInfo.getState();
                osg::GLExtensions* extensions = state->get<osg::GLExtensions>();
                extensions->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                mRemapCountSSBO->getGLBufferObject(state->getContextID())->bindBuffer();
                extensions->glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &mDrawnInstanceCount);
            }
        };

        class InstancedGeometryDrawCallback : public osg::Drawable::DrawCallback
        {
            const uint32_t& mDrawnInstanceCount;
        public:
            InstancedGeometryDrawCallback(const uint32_t& drawnInstanceCount) : mDrawnInstanceCount(drawnInstanceCount) {}

            virtual void drawImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* drawable) const
            {
                osg::DrawElements* drawElement = const_cast<osg::DrawElements*>(dynamic_cast<const osg::DrawElements*>(drawable->asGeometry()->getPrimitiveSet(0)));
                if (mDrawnInstanceCount != 0)
                {
                    drawElement->setNumInstances(mDrawnInstanceCount);
                    drawable->drawImplementation(renderInfo);
                }
            }
        };

        void createInstancedDatasTBO()
        {
            constexpr size_t initInstanceCount = 32;

            mInstancedDatasTBO = new osg::TextureBuffer;
            mInstancedDatasTBO->setInternalFormat(GL_RGBA32F);
            mInstancedDatasTBO->setSourceFormat(GL_RGBA);
            mInstancedDatasTBO->setSourceType(GL_FLOAT);
            osg::Image* image = new osg::Image;
            image->setPixelFormat(GL_RGBA);
            image->setDataType(GL_FLOAT);
            image->allocateImage(initInstanceCount * 4, 1, 1, GL_RGBA, GL_FLOAT); // 4 pixels per matrix
            mInstancedDatasTBO->setImage(image);

            mInstancedRemapTBO = new osg::TextureBuffer;
            mInstancedRemapTBO->setInternalFormat(GL_R32UI);
            mInstancedRemapTBO->setSourceFormat(GL_RED);
            mInstancedRemapTBO->setSourceType(GL_UNSIGNED_INT);
            osg::Image* remapImage = new osg::Image;
            remapImage->setPixelFormat(GL_RED);
            remapImage->setDataType(GL_UNSIGNED_INT);
            remapImage->allocateImage(initInstanceCount, 1, 1, GL_RED, GL_UNSIGNED_INT);
            mInstancedRemapTBO->setImage(remapImage);

            osg::StateSet* stateSet = mOsgLOD->getOrCreateStateSet();
            stateSet->setDefine("INSTANCED", "1");
            stateSet->addUniform(new osg::Uniform("uInstancedRemapBuffer", 14));
            stateSet->addUniform(new osg::Uniform("uInstancedDataBuffer", 15));
            stateSet->setTextureAttribute(14, mInstancedRemapTBO, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            stateSet->setTextureAttribute(15, mInstancedDatasTBO, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

            osg::ref_ptr<osg::UIntArray> remapCountBuffer = new osg::UIntArray(1);
            mRemapCountSSBO = new osg::ShaderStorageBufferObject;
            remapCountBuffer->setBufferObject(mRemapCountSSBO);

            mCullingDispatch = new osg::DispatchCompute(1, 1, 1);
            mCullingDispatch->setDrawCallback(new CullingDispatchDrawCallback(mRemapCountSSBO, mDrawnInstanceCount));
            mOsgComponentGroup->insertChild(0, mCullingDispatch);
            osg::Program* program = new osg::Program;
            program->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "Test/InstancedCulling.comp.glsl"));
            osg::StateSet* dispatchStateSet = mCullingDispatch->getOrCreateStateSet();
            dispatchStateSet->setAttribute(program, osg::StateAttribute::ON);
            dispatchStateSet->setAttribute(new osg::BindImageTexture(0, mInstancedDatasTBO, osg::BindImageTexture::READ_ONLY, GL_RGBA32F), osg::StateAttribute::ON);
            dispatchStateSet->setAttribute(new osg::BindImageTexture(1, mInstancedRemapTBO, osg::BindImageTexture::WRITE_ONLY, GL_R32UI), osg::StateAttribute::ON);
            dispatchStateSet->setAttribute(new osg::ShaderStorageBufferBinding(2, remapCountBuffer, 0, sizeof(uint32_t)), osg::StateAttribute::ON);
            mBoundingSphereRadiusUniform = new osg::Uniform("uBoundingSphereRadius", -1.0f);
            mBoundingSphereRadiusUniform->setUpdateCallback(new BoundingSphereRadiusUniformCallback(mNativeBoundingSphere));
            mInstanceCountUniform = new osg::Uniform("uInstanceCount", 0u);
            mInstanceCountUniform->setUpdateCallback(new InstanceCountUniformCallback(mInstancedDatas));
            dispatchStateSet->addUniform(mBoundingSphereRadiusUniform);
            dispatchStateSet->addUniform(mInstanceCountUniform);

            mCullingDispatch->setCullingActive(false);
            mCullingDispatch->setNodeMask(GBUFFER_MASK | SHADOW_CAST_MASK);

            updateInstancedDatasTBO();
        }

        void updateInstancedDatasTBO()
        {
            if (mInstancedDatas.empty())
                return;
            osg::Image* image = mInstancedDatasTBO->getImage();
            std::vector<osg::Matrixf> relativeMatrices(mInstancedDatas.size());
            for (size_t i = 0; i < relativeMatrices.size(); ++i)
            {
                const InstancedData& instancedData = mInstancedDatas[i];
                osg::Quat quat(
                    osg::DegreesToRadians(instancedData.rotation.x()), osg::X_AXIS,
                    osg::DegreesToRadians(instancedData.rotation.y()), osg::Y_AXIS,
                    osg::DegreesToRadians(instancedData.rotation.z()), osg::Z_AXIS
                );
                relativeMatrices[i] = osg::Matrixf::scale(instancedData.scale) * osg::Matrixf::rotate(quat) * osg::Matrixf::translate(instancedData.translation);
            }

            size_t imageDataSize = image->getTotalSizeInBytes();
            if (mInstancedDatas.size() * sizeof(osg::Matrixf) > imageDataSize)
            {
                size_t extendedCapacity = mInstancedDatas.capacity();
                size_t extendedPixelCount = extendedCapacity * sizeof(osg::Matrixf) / (4 * sizeof(float));
                image->allocateImage(extendedPixelCount, 1, 1, GL_RGBA, GL_FLOAT);
            }

            std::memcpy(image->data(), relativeMatrices.data(), relativeMatrices.size() * sizeof(osg::Matrixf));
            image->dirty();
            mInstancedDatasTBO->dirtyTextureObject();

            osg::Image* remapImage = mInstancedRemapTBO->getImage();
            if (mInstancedDatas.size() > remapImage->s())
            {
                size_t extendedCapacity = mInstancedDatas.capacity();
                remapImage->allocateImage(extendedCapacity, 1, 1, GL_RED, GL_UNSIGNED_INT);
                mInstancedRemapTBO->dirtyTextureObject();
            }
        }

        void updateInstancedData(size_t index)
        {
            osg::Image* image = mInstancedDatasTBO->getImage();
            const InstancedData& instancedData = mInstancedDatas[index];
            osg::Quat quat(
                    osg::DegreesToRadians(instancedData.rotation.x()), osg::X_AXIS,
                    osg::DegreesToRadians(instancedData.rotation.y()), osg::Y_AXIS,
                    osg::DegreesToRadians(instancedData.rotation.z()), osg::Z_AXIS
            );
            osg::Matrixf relativeMatrix = osg::Matrixf::scale(instancedData.scale) * osg::Matrixf::rotate(quat) * osg::Matrixf::translate(instancedData.translation);
            std::memcpy(image->data() + index * sizeof(osg::Matrixf), relativeMatrix.ptr(), sizeof(osg::Matrixf));
            image->dirty();
        }
    };

    namespace refl
    {
        template <> inline Type* Reflection::createType<InstancedMeshRenderer::InstancedData>()
        {
            Structure* structure = new TStructure<InstancedMeshRenderer::InstancedData>("InstancedMeshRenderer::InstancedData");
            structure->addProperty("Translation", &InstancedMeshRenderer::InstancedData::translation);
            structure->addProperty("Rotation", &InstancedMeshRenderer::InstancedData::rotation);
            structure->addProperty("Scale", &InstancedMeshRenderer::InstancedData::scale);
            return structure;
        }

        template <> inline Type* Reflection::createType<InstancedMeshRenderer>()
        {
            Class* clazz = new TClass<InstancedMeshRenderer, MeshRenderer>("InstancedMeshRenderer");
            clazz->addProperty("InstancedDatas", &InstancedMeshRenderer::mInstancedDatas);
            return clazz;
        }
    }
}
