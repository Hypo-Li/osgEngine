#pragma once
#include "Material.h"

#if 1

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

    struct LODInfo
    {
        std::vector<SubmeshView> submeshViews;
        std::vector<uint32_t> materialIndices;
        std::pair<float, float> range;
    };

    class MeshRenderer;
    class Mesh : public Object
    {
        friend class MeshRenderer;
        REFLECT_CLASS(Mesh)
    public:
        Mesh() = default;
        Mesh(const std::string& meshPath);
        Mesh(osg::Node* meshNode);

        virtual void preSave() override;

        virtual void postSave() override;

        virtual void postLoad() override;

        uint32_t getLODCount() const
        {
            return mLODInfos.size();
        }

        uint32_t getSubmeshCount(uint32_t lod) const
        {
            if (lod < mLODInfos.size())
                return mLODInfos[lod].submeshViews.size();
            return 0;
        }

        uint32_t getMaterialCount() const
        {
            return mMaterials.size();
        }

        using OsgGeometries = std::vector<osg::ref_ptr<osg::Geometry>>;
        const OsgGeometries& getOsgGeometries(uint32_t lod) const
        {
            return mOsgLODGeometries[lod];
        }

        void setMaterial(uint32_t index, Material* material)
        {
            if (index < mMaterials.size())
                mMaterials[index] = material;
        }

        void setMaterialIndex(uint32_t lod, uint32_t submesh, uint32_t index)
        {
            if (lod < mLODInfos.size() && submesh < mLODInfos[lod].materialIndices.size() && index < mMaterials.size())
                mLODInfos[lod].materialIndices[submesh] = index;
        }

        Material* getMaterial(uint32_t index) const
        {
            if (index < mMaterials.size())
                return mMaterials[index];
            return nullptr;
        }

        Material* getMaterial(uint32_t lod, uint32_t submesh) const
        {
            if (lod < mLODInfos.size() && submesh < mLODInfos[lod].materialIndices.size())
            {
                uint32_t materialIndex = mLODInfos[lod].materialIndices[submesh];
                return mMaterials[materialIndex];
            }
            return nullptr;
        }

        uint32_t getMaterialIndex(uint32_t lod, uint32_t submesh) const
        {
            if (lod < mLODInfos.size() && submesh < mLODInfos[lod].materialIndices.size())
            {
                return mLODInfos[lod].materialIndices[submesh];
            }
            return uint32_t(-1);
        }

        void setLODRange(uint32_t lod, float min, float max)
        {
            if (lod < mLODInfos.size())
                mLODInfos[lod].range = { min, max };
        }

        std::pair<float, float> getLODRange(uint32_t lod) const
        {
            if (lod < mLODInfos.size())
                return mLODInfos[lod].range;
            return { 0.0f, 0.0f };
        }

        void setDataCompression(bool compression)
        {
            mDataCompression = compression;
        }

        bool getDataCompression() const
        {
            return mDataCompression;
        }

    protected:
        bool mDataCompression = true;
        std::vector<uint8_t> mData;
        std::vector<LODInfo> mLODInfos;
        std::vector<osg::ref_ptr<Material>> mMaterials;

        std::vector<OsgGeometries> mOsgLODGeometries;

        osg::Array* createOsgArrayByVertexAttributeView(const VertexAttributeView& vav);
        osg::DrawElements* createOsgDrawElementsByIndexBufferView(const IndexBufferView& ibv);

        void compressData()
        {
            if (!mDataCompression)
                return;
            constexpr size_t bytePerMib = 1048576;
            constexpr int maxThreadCount = 12;
            int threadCount = std::min(maxThreadCount, int(mData.size() / (100 * bytePerMib)));
            std::vector<uint8_t> compressedData(mData.size());
            size_t compressedSize = FL2_compressMt(compressedData.data(), compressedData.size(), mData.data(), mData.size(), 0, threadCount);
            compressedData.resize(compressedSize);
            mData = compressedData;
        }

        void decompressData()
        {
            if (!mDataCompression)
                return;
            size_t decompressedSize = FL2_findDecompressedSize(mData.data(), mData.size());
            constexpr size_t bytePerMib = 1048576;
            constexpr int maxThreadCount = 12;
            int threadCount = std::min(maxThreadCount, int(decompressedSize / (100 * bytePerMib)));

            std::vector<uint8_t> decompressedData(decompressedSize);
            FL2_decompressMt(decompressedData.data(), decompressedSize, mData.data(), mData.size(), threadCount);
            mData = decompressedData;
        }

        void parseOsgNodes(uint32_t lod, const std::vector<osg::ref_ptr<osg::Node>>& nodes);

        void loadMesh(osg::Node* node);
    };

    namespace refl
    {
        template <> Type* Reflection::createType<VertexAttributeView>();
        template <> Type* Reflection::createType<IndexBufferView>();
        template <> Type* Reflection::createType<SubmeshView>();
        template <> Type* Reflection::createType<LODInfo>();
        template <> Type* Reflection::createType<Mesh>();
    }
}
#endif // 0

