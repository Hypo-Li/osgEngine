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
        virtual ~TextureAsset() = default;

        virtual void serialize(Json& json, std::vector<char>& binary, std::vector<std::string>& reference) const override;

        virtual void deserialize(const Json& json, const std::vector<char>& binary, const std::vector<std::string>& reference) override;

        void setTexture(osg::Texture* texture) { _texture = texture; }

    private:
        osg::ref_ptr<osg::Texture> _texture;

        static const ConstBiMap<GLenum, std::string> _sTextureTypeStringMap;
        static const ConstBiMap<GLenum, std::string> _sTextureFormatStringMap;
        static const ConstBiMap<GLenum, std::string> _sPixelFormatStringMap;
        static const ConstBiMap<GLenum, std::string> _sPixelTypeStringMap;
        static const ConstBiMap<osg::Texture::FilterMode, std::string> _sTextureFilterStringMap;
        static const ConstBiMap<osg::Texture::WrapMode, std::string> _sTextureWrapStringMap;

        static osg::ref_ptr<osg::Texture> createTextureByType(GLenum type);
    };
}
