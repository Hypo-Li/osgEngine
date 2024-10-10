#pragma once
#include <Engine/Core/Component.h>
#include <Engine/Render/Mesh.h>

#include <osg/Geode>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgUtil/CullVisitor>

#define GBUFFER_MASK        0x00000001
#define SHADOW_CAST_MASK    0x00000002

namespace xxx
{
    class MeshRenderer : public Component
    {
        REFLECT_CLASS(MeshRenderer)
    public:
        MeshRenderer() :
            mOsgGeode(new osg::Geode)
        {
            mOsgComponentGroup->addChild(mOsgGeode);
        }
        virtual ~MeshRenderer() = default;

        virtual void postSerialize(Serializer* serializer) override
        {
            if (serializer->isLoader())
            {
                syncMeshState();
            }
        }

        virtual Type getType() const override
        {
            return Type::MeshRenderer;
        }

        void setMesh(Mesh* mesh)
        {
            if (!mesh)
                return;

            mMesh = mesh;
            syncMeshState();
        }

        void syncMeshState()
        {
            if (!mMesh)
                return;

            mOsgGeometries = mMesh->generateGeometries();
            mMaterials.resize(mOsgGeometries.size());

            for (uint32_t i = 0; i < mOsgGeometries.size(); ++i)
            {
                mOsgGeode->addDrawable(mOsgGeometries[i]);
                mOsgGeometries[i]->setStateSet(getMaterial(i)->getOsgStateSet());
            }
        }

        void setMaterial(uint32_t index, Material* material)
        {
            if (index >= mOsgGeometries.size())
                return;

            mMaterials[index] = material;
            mOsgGeometries[index]->setStateSet(material->getOsgStateSet());
        }

        Mesh* getMesh()
        {
            return mMesh;
        }

        size_t getSubmeshesCount()
        {
            return mOsgGeometries.size();
        }

        Material* getMaterial(uint32_t index)
        {
            if (index >= mOsgGeometries.size())
                return nullptr;
            return mMaterials[index] ? mMaterials[index] : mMesh->getDefaultMaterial(index);
        }

    private:
        osg::ref_ptr<Mesh> mMesh;
        std::vector<osg::ref_ptr<Material>> mMaterials;

        osg::ref_ptr<osg::Geode> mOsgGeode;
        std::vector<osg::ref_ptr<osg::Geometry>> mOsgGeometries;
    };

    namespace refl
    {
        template <> inline Type* Reflection::createType<MeshRenderer>()
        {
            Class* clazz = new ClassInstance<MeshRenderer, Component>("MeshRenderer");
            clazz->addProperty("Mesh", &MeshRenderer::mMesh);
            clazz->addProperty("Materials", &MeshRenderer::mMaterials);
            return clazz;
        }
    }
}
