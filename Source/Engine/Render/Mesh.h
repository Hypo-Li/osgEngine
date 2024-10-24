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

        void setDataCompression(bool compression)
        {
            mDataCompression = compression;
        }

        bool getDataCompression() const
        {
            return mDataCompression;
        }

        std::vector<osg::ref_ptr<osg::Geometry>> generateGeometries();

    protected:
        std::vector<uint8_t> mData;
        bool mDataCompression = false;
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

        void compressData()
        {
            if (!mDataCompression)
                return;
            std::vector<uint8_t> compressedData(mData.size());
            size_t compressedSize = FL2_compress(compressedData.data(), compressedData.size(), mData.data(), mData.size(), 0);
            compressedData.resize(compressedSize);
            mData = compressedData;
        }

        void decompressData()
        {
            if (!mDataCompression)
                return;
            size_t decompressedSize = FL2_findDecompressedSize(mData.data(), mData.size());
            std::vector<uint8_t> decompressedData(decompressedSize);
            FL2_decompress(decompressedData.data(), decompressedSize, mData.data(), mData.size());
            mData = decompressedData;
        }
    };

    namespace refl
    {
        template <> Type* Reflection::createType<VertexAttributeView>();
        template <> Type* Reflection::createType<IndexBufferView>();
        template <> Type* Reflection::createType<SubmeshView>();
        template <> Type* Reflection::createType<Mesh>();
    }
}
