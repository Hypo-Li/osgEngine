#pragma once
#include "Asset.h"
#include <osg/Texture>
#include <osg/Texture2D>

namespace xxx
{
    class MaterialAsset;
    class TextureAsset : public Asset
    {
        friend class MaterialAsset;
    public:
        TextureAsset() : Asset(Type::Texture) {}

        virtual void serialize(Json& json, std::vector<char>& binray, std::vector<std::string>& reference) const override
        {
            GLenum textureType = _texture->getTextureTarget();
            json["Type"] = _sTextureTypeNameMap.at(textureType);
            json["Width"] = _texture->getTextureWidth();
            json["Height"] = _texture->getTextureHeight();
            if (textureType == GL_TEXTURE_2D_ARRAY || textureType == GL_TEXTURE_3D)
                json["Depth"] = _texture->getTextureDepth();
            json["Format"] = _texture->getInternalFormat();

            uint32_t imageCount = _texture->getNumImages();
            std::vector<Json> imagesJsonArray(imageCount);
            for (uint32_t i = 0; i < imageCount; ++i)
            {
                imagesJsonArray[i][""]
            }

        }
        virtual void deserialize(const Json& json, const std::vector<char>& binray, const std::vector<std::string>& reference) override
        {

        }

    private:
        osg::ref_ptr<osg::Texture> _texture;
    };
}
