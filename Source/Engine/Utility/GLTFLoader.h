#pragma once
#include <ThirdParty/tinygltf/tiny_gltf.h>
#include <Engine/Render/Mesh.h>

#include <osg/MatrixTransform>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Depth>
#include <osg/CullFace>
#include <osg/Texture2D>
#include <osgUtil/TangentSpaceGenerator>

#include <filesystem>

namespace xxx
{
    class GLTFLoader
    {
    public:
        static osg::Node* load(const std::string& filePath);

        static Mesh* load2(const std::string& filePath);

    private:
        struct MapCache
        {
            std::map<int, osg::ref_ptr<osg::Node>> nodeMap;
            std::map<int, osg::ref_ptr<osg::Node>> meshMap;
            std::map<int, osg::ref_ptr<osg::StateSet>> materialMap;
            std::map<int, osg::ref_ptr<osg::Texture2D>> textureMap;
            std::map<int, osg::ref_ptr<osg::Image>> imageMap;

            std::map<osg::StateSet*, osg::ref_ptr<Material>> materialOsgMap;
            std::map<osg::Texture2D*, osg::ref_ptr<Texture2D>> textureOsgMap;
        };

        enum class GLTFExtension
        {
            KHR_materials_emissive_strength,
        };

        static const std::map<std::string, GLTFExtension> sSupportedExtensionMap;

        static void copyBufferData(void* dst, const void* src, size_t size, size_t stride, size_t count);

        static osg::Node* createModel(tinygltf::Model& model);

        static Mesh* createXXXMesh(tinygltf::Model& model);

        static osg::Node* createNode(int index, tinygltf::Model& model, MapCache& cache);

        static osg::Node* createMesh(int index, tinygltf::Model& model, MapCache& cache);

        static osg::StateSet* createMaterial(int index, tinygltf::Model& model, MapCache& cache);

        static Material* createXXXMaterial(int index, tinygltf::Model& model, MapCache& cache, osg::StateSet* stateSet = nullptr);

        static osg::Texture2D* createTexture(int index, tinygltf::Model& model, MapCache& cache);

        static Texture2D* createXXXTexture2D(int index, tinygltf::Model& model, MapCache& cache, osg::Texture2D* osgTex = nullptr);

        static osg::Texture2D* createDefaultTexture(osg::Vec4 color);

        static osg::Texture2D* createDefaultWhiteTexture();

        static osg::Texture2D* createDefaultNormalTexture();

        static osg::Image* createImage(int index, tinygltf::Model& model, MapCache& cache);

        static osg::Geometry* createPrimitive(tinygltf::Primitive& primitive, tinygltf::Model& model);

    };
}
