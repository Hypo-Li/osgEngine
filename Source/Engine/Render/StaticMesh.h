#pragma once
#include "Material.h"

namespace xxx
{
    struct VertexAttributeView
    {
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

    class StaticMesh : public Object
    {
    public:


    private:
        std::vector<uint8_t> mData;
        std::vector<std::pair<uint32_t, VertexAttributeView>> mVertexAttributeViews;
        IndexBufferView mIndexBufferView;

        std::vector<osg::ref_ptr<osg::Geometry>> mOsgGeometries;
    };
}
