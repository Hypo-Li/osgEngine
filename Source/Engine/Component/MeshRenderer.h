#pragma once
#include <Engine/Core/Component.h>
#include <Engine/Render/Mesh.h>

#include <osg/LOD>
#include <osg/Geode>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgUtil/CullVisitor>

namespace xxx
{
    class MeshRenderer : public Component
    {
        REFLECT_CLASS(MeshRenderer)
    public:
        MeshRenderer();
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
            return mOverlayMaterials.size();
        }

        virtual void syncWithMesh()
        {
            if (!mMesh)
                return;

            uint32_t lodCount = mMesh->getLODCount();
            uint32_t submeshCount = mMesh->getSubmeshCount();

            mOverlayMaterials.resize(submeshCount);
            //for (uint32_t i = 0; i < submeshCount; ++i)
            //    mOverlayMaterials[i] = nullptr;

            mOsgLOD->removeChildren(0, mOsgLOD->getNumChildren());
            mOsgGeodes.resize(lodCount);
            for (uint32_t i = 0; i < lodCount; ++i)
            {
                osg::ref_ptr<osg::Group> group = new osg::Group;
                const Mesh::OsgGeometries& geometries = mMesh->getOsgGeometries(i);
                mOsgGeodes[i].resize(submeshCount);
                for (uint32_t j = 0; j < submeshCount; ++j)
                {
                    mOsgGeodes[i][j] = new osg::Geode;
                    mOsgGeodes[i][j]->addDrawable(geometries[j]);
                    Material* material = mMesh->getDefaultMaterial(j);
                    mOsgGeodes[i][j]->setStateSet(material->getOsgStateSet());
                    mOsgGeodes[i][j]->setNodeMask(material->getOsgNodeMask());
                    group->addChild(mOsgGeodes[i][j]);
                }
                mOsgLOD->addChild(group);
                std::pair<float, float> lodRange = mMesh->getLODRange(i);
                mOsgLOD->setRange(i, lodRange.first, lodRange.second);
            }
        }

        void setOverlayMaterial(uint32_t index, Material* material)
        {
            if (index >= mOverlayMaterials.size())
                return;

            mOverlayMaterials[index] = material;
            uint32_t lodCount = mMesh->getLODCount();
            for (uint32_t i = 0; i < lodCount; ++i)
            {
                mOsgGeodes[i][index]->setStateSet(material->getOsgStateSet());
                mOsgGeodes[i][index]->setNodeMask(material->getOsgNodeMask());
            }
        }

        Material* getMaterial(uint32_t index)
        {
            if (index >= mOverlayMaterials.size())
                return nullptr;
            return mOverlayMaterials[index] ? mOverlayMaterials[index] : mMesh->getDefaultMaterial(index);
        }

    protected:
        osg::ref_ptr<Mesh> mMesh;
        std::vector<osg::ref_ptr<Material>> mOverlayMaterials;

        osg::ref_ptr<osg::LOD> mOsgLOD;
        std::vector<std::vector<osg::ref_ptr<osg::Geode>>> mOsgGeodes;
    };

    namespace refl
    {
        template <> inline Type* Reflection::createType<MeshRenderer>()
        {
            Class* clazz = new TClass<MeshRenderer, Component>("MeshRenderer");
            clazz->addProperty("Mesh", &MeshRenderer::mMesh);
            clazz->addProperty("OverlayMaterials", &MeshRenderer::mOverlayMaterials);
            return clazz;
        }
    }
}
