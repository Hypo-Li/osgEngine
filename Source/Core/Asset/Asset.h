#pragma once
#include <ThirdParty/nlohmann/json.hpp>

#include <osg/GL>
#include <osg/Array>

namespace xxx
{
    using Json = nlohmann::json;
    class Asset : public osg::Referenced
    {
    public:
        enum class Type : uint32_t
        {
            Unknow      = 0x58554B4E, // XUKN
            Texture     = 0x58544558, // XTEX
            Material    = 0x584D4154, // XMAT
            StaticMesh  = 0x5853544D, // XSTM
        };

        Asset(Type type) : _type(type) {}
        virtual ~Asset() = 0;

        virtual void serialize(Json& json, std::vector<char>& binray, std::vector<std::string>& reference) const = 0;
        virtual void deserialize(const Json& json, const std::vector<char>& binary, const std::vector<std::string>& reference) = 0;

        Type getType()
        {
            return _type;
        }

        const std::string& getPath()
        {
            return _path;
        }

    protected:
        Type _type;
        std::string _path;

        static const std::unordered_map<GLenum, std::string> _sTextureTypeNameMap;
        static const std::unordered_map<std::string, GLenum> _sTextureNameTypeMap;
        static const std::unordered_map<osg::Array::Type, std::string> _sArrayTypeNameMap;
        static const std::unordered_map<std::string, osg::Array::Type> _sArrayNameTypeMap;

        static osg::ref_ptr<osg::Texture> createTextureByType(GLenum type);
        static osg::ref_ptr<osg::Array> createArrayByType(osg::Array::Type type);

        static size_t getReferenceIndex(const std::string& assetPath, std::vector<std::string>& reference);
    };
}
