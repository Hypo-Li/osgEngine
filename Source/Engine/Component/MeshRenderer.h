#pragma once
#include <Engine/Core/Component.h>
#include <Engine/Render/Mesh.h>

#include <osg/LOD>
#include <osg/Geode>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgUtil/CullVisitor>
#include <osgUtil/Optimizer>

namespace xxx
{
    class ShadowGroup : public osg::Group
    {
        std::vector<osg::ref_ptr<osg::StateSet>> mChildrenStateSets;
    public:
        virtual void traverse(osg::NodeVisitor& nv) override
        {
            if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
            {
                if (mChildrenStateSets.size() != _children.size())
                {
                    mChildrenStateSets.resize(_children.size());
                    for (uint32_t i = 0; i < _children.size(); ++i)
                        mChildrenStateSets[i] = _children[i]->getStateSet();
                }

                uint32_t cullMask = nv.asCullVisitor()->getCurrentCamera()->getCullMask();
                if (cullMask == SHADOW_CAST_MASK)
                {
                    for (uint32_t i = 0; i < _children.size(); ++i)
                    {
                        if (_children[i]->getStateSet() != nullptr)
                        {
                            mChildrenStateSets[i] = _children[i]->getStateSet();
                            _children[i]->setStateSet(nullptr);
                        }
                    }
                }
                else
                {
                    for (uint32_t i = 0; i < _children.size(); ++i)
                    {
                        if (_children[i]->getStateSet() == nullptr)
                            _children[i]->setStateSet(mChildrenStateSets[i]);
                        else
                            mChildrenStateSets[i] = _children[i]->getStateSet();
                    }
                }
            }

            Group::traverse(nv);
        }
    };

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
            if (!mesh || mMesh == mesh)
                return;

            mMesh = mesh;
            syncWithMesh();
        }

        Mesh* getMesh()
        {
            return mMesh;
        }

        size_t getSubmeshCount(uint32_t lod = 0)
        {
            if (mMesh)
                return mMesh->getSubmeshCount(lod);
            return 0;
        }

        virtual void syncWithMesh()
        {
            if (!mMesh)
                return;

            mOsgLOD->removeChildren(0, mOsgLOD->getNumChildren());

            mOverlayMaterials.resize(mMesh->getMaterialCount());

            uint32_t lodCount = mMesh->getLODCount();
            mOsgGeodes.resize(lodCount);
            for (uint32_t lod = 0; lod < lodCount; ++lod)
            {
                uint32_t submeshCount = mMesh->getSubmeshCount(lod);
                osg::ref_ptr<osg::Group> group = new ShadowGroup;
                const Mesh::OsgGeometries& geometries = mMesh->getOsgGeometries(lod);
                mOsgGeodes[lod].resize(submeshCount);
                for (uint32_t submesh = 0; submesh < submeshCount; ++submesh)
                {
                    Material* material = mMesh->getMaterial(lod, submesh);

                    osg::Geode* geode = new osg::Geode;
                    geode->addDrawable(geometries[submesh]);
                    geode->setStateSet(material->getOsgStateSet());
                    geode->setNodeMask(material->getOsgNodeMask());
                    group->addChild(geode);
                    geode->computeBound();
                    mOsgGeodes[lod][submesh] = geode;
                }

                mOsgLOD->addChild(group);
                std::pair<float, float> lodRange = mMesh->getLODRange(lod);
                mOsgLOD->setRange(lod, lodRange.first, lodRange.second);
            }
        }

        void setOverlayMaterial(uint32_t index, Material* material)
        {
            if (index >= mOverlayMaterials.size() || !mMesh)
                return;

            mOverlayMaterials[index] = material;
            uint32_t lodCount = mMesh->getLODCount();
            for (uint32_t lod = 0; lod < lodCount; ++lod)
            {
                uint32_t submeshCount = mMesh->getSubmeshCount(lod);
                for (uint32_t submesh = 0; submesh < submeshCount; ++submesh)
                {
                    uint32_t materialIndex = mMesh->getMaterialIndex(lod, submesh);
                    if (materialIndex == index)
                    {
                        mOsgGeodes[lod][submesh]->setStateSet(material->getOsgStateSet());
                        mOsgGeodes[lod][submesh]->setNodeMask(material->getOsgNodeMask());
                    }
                }
            }
        }

        Material* getMaterial(uint32_t index) const
        {
            if (index >= mOverlayMaterials.size())
                return nullptr;
            return mOverlayMaterials[index] ? mOverlayMaterials[index].get() : mMesh->getMaterial(index);
        }

        uint32_t getMaterialCount() const
        {
            return mOverlayMaterials.size();
        }

    protected:
        osg::ref_ptr<Mesh> mMesh;
        std::vector<osg::ref_ptr<Material>> mOverlayMaterials;

        osg::ref_ptr<osg::LOD> mOsgLOD;
        std::vector<std::vector<osg::ref_ptr<osg::Geode>>> mOsgGeodes;

        osg::Matrixf mPrevFrameMVMatrix;
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
