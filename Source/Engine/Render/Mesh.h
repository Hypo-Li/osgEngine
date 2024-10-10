#pragma once
#include "Material.h"

namespace xxx
{
    struct VertexAttributeView
    {
        uint32_t location;
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

    struct SubmeshView
    {
        std::vector<VertexAttributeView> vertexAttributeViews;
        IndexBufferView indexBufferView;
    };

    class MeshRenderer;
    class Mesh : public Object
    {
        friend class MeshRenderer;
        REFLECT_CLASS(Mesh)
    public:
        Mesh() = default;
        Mesh(const std::string& meshPath);

        virtual void preSerialize(Serializer* serializer) override;

        virtual void postSerialize(Serializer* serializer) override;

        uint32_t getSubmeshCount() const
        {
            return mOsgGeometryDatas.size();
        }

        void setDefaultMaterial(uint32_t index, Material* material)
        {
            if (index >= mDefaultMaterials.size())
                return;
            mDefaultMaterials[index] = material;
        }

        Material* getDefaultMaterial(uint32_t index)
        {
            if (index >= mDefaultMaterials.size())
                return nullptr;
            return mDefaultMaterials[index];
        }

        std::vector<osg::ref_ptr<osg::Geometry>> generateGeometries();

    protected:
        std::vector<uint8_t> mData;
        std::vector<SubmeshView> mSubmeshViews;
        std::vector<osg::ref_ptr<Material>> mDefaultMaterials;

        struct OsgGeometryData
        {
            std::vector<std::pair<uint32_t, osg::ref_ptr<osg::Array>>> vertexAttributes;
            osg::ref_ptr<osg::DrawElements> drawElements;
        };
        std::vector<OsgGeometryData> mOsgGeometryDatas;

        static osg::Array* createOsgArrayByVertexAttributeView(VertexAttributeView& vav, uint8_t* data);
        static osg::DrawElements* createOsgDrawElementsByIndexBufferView(IndexBufferView& ibv, uint8_t* data);
    };

    namespace refl
    {
        template <> Type* Reflection::createType<VertexAttributeView>();

        template <> Type* Reflection::createType<IndexBufferView>();

        template <> Type* Reflection::createType<SubmeshView>();

        template <> Type* Reflection::createType<Mesh>();
    }
}
