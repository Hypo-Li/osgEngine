#pragma once
#include "MeshRenderer.h"

#include <osg/TextureBuffer>

namespace xxx
{
    class InstancedMeshRenderer : public MeshRenderer
    {
        REFLECT_CLASS(InstancedMeshRenderer)
    public:
        struct InstanceData
        {
            osg::Vec3f translation;
            osg::Vec3f rotation;
            osg::Vec3f scale;
        };

        struct InstancedDrawableComputeBoundingBoxCallback : public osg::Drawable::ComputeBoundingBoxCallback
        {
            const std::vector<InstanceData>& mInstanceDatas;
            mutable osg::BoundingBox mNativeBoundingBox;

            InstancedDrawableComputeBoundingBoxCallback(const std::vector<InstanceData>& instanceDatas) : mInstanceDatas(instanceDatas) {}

            virtual osg::BoundingBox computeBound(const osg::Drawable& drawable) const
            {
                osg::BoundingBox result;
                if (!mNativeBoundingBox.valid())
                    mNativeBoundingBox = drawable.computeBoundingBox();

                for (const auto& instanceData : mInstanceDatas)
                {
                    osg::Quat quat(
                        osg::DegreesToRadians(instanceData.rotation.x()), osg::X_AXIS,
                        osg::DegreesToRadians(instanceData.rotation.y()), osg::Y_AXIS,
                        osg::DegreesToRadians(instanceData.rotation.z()), osg::Z_AXIS
                    );
                    osg::Matrixf relativeMatrix = osg::Matrixf::scale(instanceData.scale) * osg::Matrixf::rotate(quat) * osg::Matrixf::translate(instanceData.translation);

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
            createInstanceDatasTBO();
            syncWithMesh();
        }

        virtual void onDisable() override
        {

        }

        virtual void syncWithMesh() override
        {
            MeshRenderer::syncWithMesh();
            for (const auto& lodGeodes : mOsgGeodes)
            {
                for (osg::Geode* geode : lodGeodes)
                {
                    osg::Geometry* geometry = dynamic_cast<osg::Geometry*>(geode->getDrawable(0));
                    osg::Geometry* instancedGeometry = new osg::Geometry(*geometry);
                    instancedGeometry->setComputeBoundingBoxCallback(new InstancedDrawableComputeBoundingBoxCallback(mInstanceDatas));
                    osg::DrawElements* instancedDrawElements = dynamic_cast<osg::DrawElements*>(instancedGeometry->getPrimitiveSet(0)->clone(osg::CopyOp::SHALLOW_COPY));
                    instancedDrawElements->setNumInstances(mInstanceDatas.size());
                    instancedGeometry->setPrimitiveSet(0, instancedDrawElements);
                    geode->setDrawable(0, instancedGeometry);
                }
            }
            if (mInstanceDatas.empty())
                mOsgLOD->setNodeMask(0);
        }

        void addInstance(osg::Vec3f translation = osg::Vec3f(0, 0, 0), osg::Vec3f rotation = osg::Vec3f(0, 0, 0), osg::Vec3f scale = osg::Vec3f(1, 1, 1))
        {
            if (mInstanceDatas.empty())
                mOsgLOD->setNodeMask(~0);
            mInstanceDatas.emplace_back(InstanceData{ translation, rotation, scale });

            for (const auto& lodGeodes : mOsgGeodes)
            {
                for (osg::Geode* geode : lodGeodes)
                {
                    osg::Geometry* geometry = dynamic_cast<osg::Geometry*>(geode->getDrawable(0));
                    osg::DrawElements* instancedDrawElements = dynamic_cast<osg::DrawElements*>(geometry->getPrimitiveSet(0));
                    instancedDrawElements->setNumInstances(mInstanceDatas.size());

                    geometry->dirtyBound();
                }
            }

            mOsgLOD->dirtyBound();
            updateInstanceDatasTBO();
        }

        void setInstance(size_t index, osg::Vec3f translation = osg::Vec3f(0, 0, 0), osg::Vec3f rotation = osg::Vec3f(0, 0, 0), osg::Vec3f scale = osg::Vec3f(1, 1, 1))
        {
            if (index >= mInstanceDatas.size())
                return;
            mInstanceDatas[index].translation = translation;
            mInstanceDatas[index].rotation = rotation;
            mInstanceDatas[index].scale = scale;

            for (const auto& lodGeodes : mOsgGeodes)
                for (osg::Geode* geode : lodGeodes)
                    geode->getDrawable(0)->dirtyBound();

            mOsgLOD->dirtyBound();
            updateInstanceData(index);
        }

        const std::vector<InstanceData>& getInstanceDatas() const
        {
            return mInstanceDatas;
        }

    protected:
        std::vector<InstanceData> mInstanceDatas;

        osg::ref_ptr<osg::TextureBuffer> mInstanceDatasTBO;

        void createInstanceDatasTBO()
        {
            mInstanceDatasTBO = new osg::TextureBuffer;
            mInstanceDatasTBO->setInternalFormat(GL_RGBA32F);
            mInstanceDatasTBO->setSourceFormat(GL_RGBA);
            mInstanceDatasTBO->setSourceType(GL_FLOAT);
            osg::Image* image = new osg::Image;
            image->setPixelFormat(GL_RGBA);
            image->setDataType(GL_FLOAT);
            mInstanceDatasTBO->setImage(image);

            osg::StateSet* stateSet = mOsgLOD->getOrCreateStateSet();
            stateSet->setDefine("INSTANCED", "1");
            stateSet->setTextureAttribute(15, mInstanceDatasTBO, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            stateSet->addUniform(new osg::Uniform("uInstancedData", 15));

            updateInstanceDatasTBO();
        }

        void updateInstanceDatasTBO()
        {
            if (mInstanceDatas.empty())
                return;
            osg::Image* image = mInstanceDatasTBO->getImage();
            std::vector<osg::Matrixf> relativeMatrices(mInstanceDatas.size());
            for (size_t i = 0; i < relativeMatrices.size(); ++i)
            {
                const InstanceData& instanceData = mInstanceDatas[i];
                osg::Quat quat(
                    osg::DegreesToRadians(instanceData.rotation.x()), osg::X_AXIS,
                    osg::DegreesToRadians(instanceData.rotation.y()), osg::Y_AXIS,
                    osg::DegreesToRadians(instanceData.rotation.z()), osg::Z_AXIS
                );
                relativeMatrices[i] = osg::Matrixf::scale(instanceData.scale) * osg::Matrixf::rotate(quat) * osg::Matrixf::translate(instanceData.translation);
            }

            size_t imageDataSize = image->getTotalSizeInBytes();
            if (mInstanceDatas.size() * sizeof(osg::Matrixf) > imageDataSize)
            {
                size_t extendedCapacity = mInstanceDatas.capacity();
                size_t extendedPixelCount = extendedCapacity * sizeof(osg::Matrixf) / (4 * sizeof(float));
                image->allocateImage(extendedPixelCount, 1, 1, GL_RGBA, GL_FLOAT);
            }

            std::memcpy(image->data(), relativeMatrices.data(), relativeMatrices.size() * sizeof(osg::Matrixf));
            mInstanceDatasTBO->dirtyTextureObject();
        }

        void updateInstanceData(size_t index)
        {
            osg::Image* image = mInstanceDatasTBO->getImage();
            const InstanceData& instanceData = mInstanceDatas[index];
            osg::Quat quat(
                    osg::DegreesToRadians(instanceData.rotation.x()), osg::X_AXIS,
                    osg::DegreesToRadians(instanceData.rotation.y()), osg::Y_AXIS,
                    osg::DegreesToRadians(instanceData.rotation.z()), osg::Z_AXIS
            );
            osg::Matrixf relativeMatrix = osg::Matrixf::scale(instanceData.scale) * osg::Matrixf::rotate(quat) * osg::Matrixf::translate(instanceData.translation);
            std::memcpy(image->data() + index * sizeof(osg::Matrixf), relativeMatrix.ptr(), sizeof(osg::Matrixf));
            image->dirty();
        }
    };

    namespace refl
    {
        template <> inline Type* Reflection::createType<InstancedMeshRenderer::InstanceData>()
        {
            Structure* structure = new TStructure<InstancedMeshRenderer::InstanceData>("InstancedMeshRenderer::InstanceData");
            structure->addProperty("Translation", &InstancedMeshRenderer::InstanceData::translation);
            structure->addProperty("Rotation", &InstancedMeshRenderer::InstanceData::rotation);
            structure->addProperty("Scale", &InstancedMeshRenderer::InstanceData::scale);
            return structure;
        }

        template <> inline Type* Reflection::createType<InstancedMeshRenderer>()
        {
            Class* clazz = new TClass<InstancedMeshRenderer, MeshRenderer>("InstancedMeshRenderer");
            clazz->addProperty("InstanceDatas", &InstancedMeshRenderer::mInstanceDatas);
            return clazz;
        }
    }
}
