#pragma once
#include "MeshRenderer.h"

#include <osg/TextureBuffer>

namespace xxx
{
    class InstancedMeshRenderer : public MeshRenderer
    {
    public:
        InstancedMeshRenderer()
        {
            createInstanceDatasTBO();

        }

        virtual void postSerializer(Serializer* serializer)
        {
            MeshRenderer::postSerialize(serializer);
            if (serializer->isLoader())
            {
                updateInstanceDatasTBO();
            }
        }

        virtual void syncWithMesh() override
        {
            MeshRenderer::syncWithMesh();
            for (osg::Geometry* geometry : mOsgGeometries)
            {
                geometry->getPrimitiveSet(0)->setNumInstances(mInstanceDatas.size());
            }
        }

        void addInstance(osg::Matrixf relativeTransformMatrix)
        {
            //mInstanceDatas.push_back();
        }

    protected:
        struct InstanceData
        {
            osg::Matrixf relativeTransformMatrix;
        };
        std::vector<InstanceData> mInstanceDatas;

        osg::ref_ptr<osg::TextureBuffer> mInstanceDatasTBO;

        void createInstanceDatasTBO()
        {
            mInstanceDatasTBO = new osg::TextureBuffer;
            mInstanceDatasTBO->setInternalFormat(GL_RGBA32F);
            mInstanceDatasTBO->setImage(new osg::Image);
            updateInstanceDatasTBO();
        }

        void updateInstanceDatasTBO()
        {
            if (mInstanceDatas.empty())
                return;
            osg::Image* image = mInstanceDatasTBO->getImage();

            size_t imageDataSize = image->getTotalSizeInBytes();
            if (mInstanceDatas.size() * sizeof(InstanceData) > imageDataSize)
            {
                size_t extendedCapacity = mInstanceDatas.capacity();
                size_t extendedPixelCount = extendedCapacity * sizeof(InstanceData) / (4 * sizeof(float));
                image->allocateImage(extendedPixelCount, 1, 1, GL_RGBA, GL_FLOAT);
            }

            std::memcpy(image->data(), mInstanceDatas.data(), mInstanceDatas.size() * sizeof(InstanceData));
            mInstanceDatasTBO->dirtyTextureObject();
        }
    };
}
