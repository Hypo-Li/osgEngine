#pragma once
#include <Core/Base/Component.h>
#include <Core/Asset/MeshAsset.h>

#include <osg/Geode>
#include <osg/Geometry>
#include <osgUtil/CullVisitor>

#define GBUFFER_MASK        0x00000001
#define SHADOW_CAST_MASK    0x00000002

namespace xxx
{
    class MeshRenderer : public Component
    {
    public:
        MeshRenderer() : _geode(new osg::Geode)
        {
            Group::addChild(_geode);
        }
        virtual ~MeshRenderer() = default;

        void setMesh(MeshAsset* mesh)
        {
            if (_mesh)
            {
                _geode->removeDrawables(0, _mesh->getSubmeshCount());
                _materials.clear();
                _geometries.clear();
            }

            if (!mesh)
                return;

            _mesh = mesh;
            uint32_t submeshCount = _mesh->getSubmeshCount();
            _materials.resize(submeshCount);
            _geometries.resize(submeshCount);

            for (uint32_t i = 0; i < submeshCount; ++i)
            {
                osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
                _geometries[i] = geometry;
                _geode->addDrawable(_geometries[i]);

                // set vertices and indices
                MeshAsset::Submesh& submesh = _mesh->_submeshes[i];
                for (auto& vertexAttribute : submesh._vertexAttributes)
                {
                    geometry->setVertexAttribArray(vertexAttribute.first, vertexAttribute.second);
                }
                geometry->addPrimitiveSet(submesh._drawElements);

                // set preview material
                geometry->setCullCallback(new SwitchMaterialCullCallback(submesh._previewMaterial));
                _materials[i].first = submesh._previewMaterial;
                _materials[i].second = false;
            }
        }

        void setMaterial(MaterialAsset* material, uint32_t index)
        {
            if (_materials.size() <= index)
                return;
            _materials[index].first = material;
            _materials[index].second = true;
            SwitchMaterialCullCallback* cullCallback = dynamic_cast<SwitchMaterialCullCallback*>(_geometries[index]->getCullCallback());
            cullCallback->setMaterial(material);
        }

        uint32_t getSubmeshCount()
        {
            return _mesh ? _mesh->getSubmeshCount() : 0;
        }

        MaterialAsset* getMaterial(uint32_t index)
        {
            return _materials.size() > index ? _materials[index].first : nullptr;
        }

    private:
        osg::ref_ptr<MeshAsset> _mesh;
        // first is material asset, second means whether the material has been set
        std::vector<std::pair<osg::ref_ptr<MaterialAsset>, bool>> _materials;
        osg::ref_ptr<osg::Geode> _geode;
        std::vector<osg::ref_ptr<osg::Geometry>> _geometries;

        class SwitchMaterialCullCallback : public osg::DrawableCullCallback
        {
            osg::ref_ptr<xxx::MaterialAsset> _material;
            osg::ref_ptr<osg::StateSet> _gbufferStateSet;
            osg::ref_ptr<osg::StateSet> _shadowCastStateSet;
        public:
            SwitchMaterialCullCallback(xxx::MaterialAsset* material) : _material(material)
            {
                applyMaterial();
            }

            virtual bool cull(osg::NodeVisitor* nv, osg::Drawable* drawable, osg::RenderInfo* renderInfo) const
            {
                switch (nv->asCullVisitor()->getCurrentCamera()->getCullMask())
                {
                case GBUFFER_MASK:
                    drawable->setStateSet(_gbufferStateSet);
                    break;
                case SHADOW_CAST_MASK:
                    drawable->setStateSet(_shadowCastStateSet);
                    break;
                default:
                    break;
                }
            }

            void setMaterial(xxx::MaterialAsset* material)
            {
                _material = material;
                applyMaterial();
            }

            const xxx::MaterialAsset* getMaterial() const
            {
                return _material;
            }

            void applyMaterial()
            {
                if (_material->getType() == Asset::Type::MaterialInstance)
                    dynamic_cast<MaterialInstanceAsset*>(_material.get())->syncMaterialTemplate();

                osg::ref_ptr<osg::Program> gbufferProgram = new osg::Program;
                gbufferProgram->addShader(_material->getShader());

                _gbufferStateSet = new osg::StateSet(*_material->getStateSet());
                _gbufferStateSet->setAttribute(gbufferProgram, osg::StateAttribute::ON);

                osg::ref_ptr<osg::Program> shadowCastProgram = new osg::Program;
                shadowCastProgram->addShader(_material->getShader());

                _shadowCastStateSet = new osg::StateSet(*_material->getStateSet());
                _gbufferStateSet->setAttribute(shadowCastProgram, osg::StateAttribute::ON);

            }
        };
    };
}
