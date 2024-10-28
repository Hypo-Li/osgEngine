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

        virtual void onEnable() override
        {
            syncWithMesh();
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
            syncWithMesh();
        }

        Mesh* getMesh()
        {
            return mMesh;
        }

        size_t getSubmeshCount()
        {
            return mOsgGeometries.size();
        }

        virtual void syncWithMesh()
        {
            if (!mMesh)
                return;

            mOsgGeometries = mMesh->generateGeometries();
            mOverlayMaterials.resize(mOsgGeometries.size());
            for (uint32_t i = 0; i < mOverlayMaterials.size(); ++i)
                mOverlayMaterials[i] = nullptr;

            mOsgGeode->removeDrawables(0, mOsgGeode->getNumDrawables());
            for (uint32_t i = 0; i < mOsgGeometries.size(); ++i)
            {
                mOsgGeode->addDrawable(mOsgGeometries[i]);
                Material* material = getMaterial(i);
                mOsgGeometries[i]->setStateSet(material->getOsgStateSet());
                mOsgGeometries[i]->setNodeMask(material->getOsgNodeMask());
            }
        }

        void setOverlayMaterial(uint32_t index, Material* material)
        {
            if (index >= mOsgGeometries.size())
                return;

            mOverlayMaterials[index] = material;
            mOsgGeometries[index]->setStateSet(material->getOsgStateSet());
            mOsgGeometries[index]->setNodeMask(material->getOsgNodeMask());
        }

        Material* getMaterial(uint32_t index)
        {
            if (index >= mOsgGeometries.size())
                return nullptr;
            return mOverlayMaterials[index] ? mOverlayMaterials[index] : mMesh->getDefaultMaterial(index);
        }

    protected:
        osg::ref_ptr<Mesh> mMesh;
        std::vector<osg::ref_ptr<Material>> mOverlayMaterials;

        osg::ref_ptr<osg::Geode> mOsgGeode;
        std::vector<osg::ref_ptr<osg::Geometry>> mOsgGeometries;
    };

    namespace refl
    {
        template <> inline Type* Reflection::createType<MeshRenderer>()
        {
            Class* clazz = new ClassInstance<MeshRenderer, Component>("MeshRenderer");
            clazz->addProperty("Mesh", &MeshRenderer::mMesh);
            clazz->addProperty("OverlayMaterials", &MeshRenderer::mOverlayMaterials);
            return clazz;
        }
    }
}
