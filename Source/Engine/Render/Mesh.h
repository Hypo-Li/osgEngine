#pragma once
#include "Material.h"

namespace xxx
{
    struct VertexAttributeView
    {
        uint32_t index;
        bool bindPerVertex;
        uint8_t dimension;
        GLenum type;
        size_t offset;
        size_t size;
    };

    struct IndexBufferView
    {
        GLenum type;
        size_t offset;
        size_t size;
    };

    struct Submesh
    {
        std::vector<VertexAttributeView> vertexAttributeViews;
        IndexBufferView indexBufferView;
        osg::ref_ptr<Material> defaultMaterial;
    };

    class MeshRenderer;
    class Mesh : public Object
    {
        friend class MeshRenderer;
        REFLECT_CLASS(Mesh)
    public:
        Mesh() = default;
        Mesh(const std::string& meshPath);

        virtual void preSerialize() override;

        virtual void postSerialize() override;

    protected:
        std::vector<uint8_t> mData;
        std::vector<Submesh> mSubmeshes;

        std::vector<osg::ref_ptr<osg::Geometry>> mOsgGeometries;

        static osg::Array* createOsgArrayByVertexAttributeView(VertexAttributeView& vav, uint8_t* data);
        static osg::DrawElements* createOsgDrawElementsByIndexBufferView(IndexBufferView& ibv, uint8_t* data);
    };

    namespace refl
    {
        template <> inline Type* Reflection::createType<VertexAttributeView>()
        {
            Struct* structure = new StructInstance<VertexAttributeView>("VertexAttributeView");
            structure->addProperty("Index", &VertexAttributeView::index);
            structure->addProperty("Dimension", &VertexAttributeView::dimension);
            structure->addProperty("Type", &VertexAttributeView::type);
            structure->addProperty("Offset", &VertexAttributeView::offset);
            structure->addProperty("Size", &VertexAttributeView::size);
            return structure;
        }

        template <> inline Type* Reflection::createType<IndexBufferView>()
        {
            Struct* structure = new StructInstance<IndexBufferView>("IndexBufferView");
            structure->addProperty("Type", &IndexBufferView::type);
            structure->addProperty("Offset", &IndexBufferView::offset);
            structure->addProperty("Size", &IndexBufferView::size);
            return structure;
        }

        template <> inline Type* Reflection::createType<Submesh>()
        {
            Struct* structure = new StructInstance<Submesh>("Submesh");
            structure->addProperty("VertexAttributeViews", &Submesh::vertexAttributeViews);
            structure->addProperty("IndexBufferView", &Submesh::indexBufferView);
            structure->addProperty("DefaultMaterial", &Submesh::defaultMaterial);
            return structure;
        }

        template <> inline Type* Reflection::createType<Mesh>()
        {
            Class* clazz = new ClassInstance<Mesh>("Mesh");
            clazz->addProperty("Data", &Mesh::mData);
            clazz->addProperty("Submeshes", &Mesh::mSubmeshes);
            getClassMap().emplace("Mesh", clazz);
            return clazz;
        }
    }
}
