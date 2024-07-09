#pragma once
#include <Core/Base/Component.h>
#include <Core/Asset/MeshAsset.h>

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
    public:
        MeshRenderer() : Component(Type::MeshRenderer) {}
        virtual ~MeshRenderer() = default;

        void setMesh(MeshAsset* mesh)
        {
            if (!mesh || _mesh == mesh)
                return;

            _materials.clear();
            _groups.clear();
            _geometries.clear();

            _mesh = mesh;
            uint32_t submeshCount = _mesh->getSubmeshesCount();
            _materials.resize(submeshCount);
            _groups.resize(submeshCount);
            _geometries.resize(submeshCount);

            for (uint32_t i = 0; i < submeshCount; ++i)
            {
                // set vertices and indices
                MeshAsset::Submesh& submesh = _mesh->_submeshes[i];

                _materials[i].first = submesh._previewMaterial;
                _materials[i].second = false;

                _groups[i] = new osg::Group;
                Group::addChild(_groups[i]);

                osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
                for (auto& vertexAttribute : submesh._vertexAttributes)
                {
                    geometry->setVertexAttribArray(vertexAttribute.first, vertexAttribute.second);
                    geometry->setVertexAttribBinding(vertexAttribute.first, osg::Geometry::BIND_PER_VERTEX);
                }
                geometry->addPrimitiveSet(submesh._drawElements);
                _geometries[i] = geometry;

                osg::ref_ptr<osg::Geode> gbufferGeode = new osg::Geode;
                _groups[i]->addChild(gbufferGeode);
                gbufferGeode->addDrawable(_geometries[i]);
                gbufferGeode->setNodeMask(0x00000001);
                osg::ref_ptr<osg::Program> gbufferProgram = new osg::Program;
                gbufferProgram->addShader(getGBufferVertexShader());
                gbufferProgram->addShader(getGBufferFragmentShader());
                gbufferProgram->addShader(_materials[i].first->getShader());
                gbufferGeode->getOrCreateStateSet()->setAttribute(gbufferProgram, osg::StateAttribute::ON);

                // shadow, outline...
            }
        }

        void setMaterial(uint32_t index, MaterialAsset* material)
        {
            if (_materials.size() <= index)
                return;
            _materials[index].first = material;
            _materials[index].second = true;

            osg::StateSet* gbufferGeodeStateSet = _groups[index]->getChild(0)->getStateSet();
            gbufferGeodeStateSet->removeAttribute(osg::StateAttribute::PROGRAM, 0);
            osg::ref_ptr<osg::Program> gbufferProgram = new osg::Program;
            gbufferProgram->addShader(getGBufferVertexShader());
            gbufferProgram->addShader(getGBufferFragmentShader());
            gbufferProgram->addShader(_materials[index].first->getShader());
            gbufferGeodeStateSet->setAttribute(gbufferProgram, osg::StateAttribute::ON);
        }

        uint32_t getSubmeshesCount()
        {
            return _mesh ? _mesh->getSubmeshesCount() : 0;
        }

        MaterialAsset* getMaterial(uint32_t index)
        {
            return _materials.size() > index ? _materials[index].first : nullptr;
        }

    private:
        osg::ref_ptr<MeshAsset> _mesh;
        // first is material asset, second means whether the material has been set
        std::vector<std::pair<osg::ref_ptr<MaterialAsset>, bool>> _materials;
        std::vector<osg::ref_ptr<osg::Group>> _groups;
        std::vector<osg::ref_ptr<osg::Geometry>> _geometries;

        static osg::ref_ptr<osg::Shader> getGBufferVertexShader()
        {
            static osg::ref_ptr<osg::Shader> gbufferVertexShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Mesh/Mesh.vert.glsl");
            return gbufferVertexShader;
        }
        static osg::ref_ptr<osg::Shader> getGBufferFragmentShader()
        {
            static osg::ref_ptr<osg::Shader> gbufferFragmentShader = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Mesh/Mesh.frag.glsl");
            return gbufferFragmentShader;
        }
    };
}
