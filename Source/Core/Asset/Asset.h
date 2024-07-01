#pragma once
#include <ThirdParty/nlohmann/json.hpp>

#include <osg/GL>
#include <osg/Array>
#include <osg/Texture>

#define LITTLE_ENDIAN 0
#define BIG_ENDIAN 1
#define BYTE_ORDER LITTLE_ENDIAN

namespace xxx
{
    using Json = nlohmann::json;
    class AssetManager;
    class Asset : public osg::Referenced
    {
        friend class AssetManager;
    public:
        enum class Type : uint32_t
        {
#if (BYTE_ORDER == BIG_ENDIAN)
            Unknow      = 0x58554B4E, // XUKN
            Texture     = 0x58544558, // XTEX
            Material    = 0x584D4154, // XMAT
            StaticMesh  = 0x5853544D, // XSTM
#else
            Unknow      = 0x4E4B5558, // XUKN
            Texture     = 0x58455458, // XTEX
            Material    = 0x54414D58, // XMAT
            StaticMesh  = 0x4D545358, // XSTM
#endif
        };

        Asset(Type type) : _type(type) {}
        virtual ~Asset() = default;

        virtual void serialize(Json& json, std::vector<char>& binary, std::vector<std::string>& reference) const = 0;
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

        template <typename T1, typename T2>
        class ConstBiMap
        {
        public:
            ConstBiMap(std::initializer_list<std::pair<T1, T2>> initList)
            {
                for (const std::pair<T1, T2>& pair : initList)
                {
                    _ForwardMap[pair.first] = pair.second;
                    _BackwardMap[pair.second] = pair.first;
                }
            }

            const T2& forwardAt(const T1& key) const
            {
                return _ForwardMap.at(key);
            }

            const T1& backwardAt(const T2& key) const
            {
                return _BackwardMap.at(key);
            }

        private:
            std::unordered_map<T1, T2> _ForwardMap;
            std::unordered_map<T2, T1> _BackwardMap;
        };

        static size_t getReferenceIndex(const std::string& assetPath, std::vector<std::string>& reference);
    };
}
