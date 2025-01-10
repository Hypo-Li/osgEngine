#if 1
#include <Engine/Core/Engine.h>
#include <Engine/Render/Pipeline.h>
#include <Engine/Core/Asset.h>
#include <Engine/Core/AssetManager.h>
#include <Engine/Component/MeshRenderer.h>
#include <Engine/Component/Light.h>

#include <osgViewer/CompositeViewer>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osg/MatrixTransform>
#include <osg/BufferObject>
#include <osg/BufferIndexBinding>
#include <osg/Texture3D>
#include <osg/DispatchCompute>
#include <osg/BindImageTexture>
#include <osg/io_utils>
#include <osg/LineWidth>
#include <osgDB/ConvertUTF>

#include <osgViewer/ViewerEventHandlers>
#include <osgParticle/ParticleSystem>
#include <osgParticle/ModularEmitter>
#include <osgParticle/ParticleSystemUpdater>
#include <osgParticle/SegmentPlacer>
#include <osgParticle/BoxPlacer>
#include <osgParticle/PrecipitationEffect>

#include <filesystem>

using namespace xxx;

class VBOVisitor : public osg::NodeVisitor
{
public:
    VBOVisitor() : NodeVisitor(TRAVERSE_ALL_CHILDREN) {}

    void apply(osg::Geometry& geometry) override
    {
        geometry.setUseVertexBufferObjects(true);
        traverse(geometry);
    }
};

int main()
{
    VBOVisitor vbov;
    for (const auto& file : std::filesystem::recursive_directory_iterator(R"(D:\Data\OropTest\Tile_+006_+010)"))
    {
        if (file.is_regular_file())
        {
            std::string path = osgDB::convertStringFromCurrentCodePageToUTF8(file.path().string());
            osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(path);
            node->accept(vbov);
            osgDB::writeObjectFile(*node, file.path().string());
        }
    }

    return 0;
}

#endif // 0
