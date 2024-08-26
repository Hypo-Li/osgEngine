#pragma once
#include "Object.h"

#include <osg/Referenced>
#include <osg/ref_ptr>
#include <string>

namespace xxx
{
    class Asset : public osg::Referenced
    {
    public:
        Asset() = default;
        virtual ~Asset() = default;

        template <typename T, std::enable_if_t<std::is_base_of_v<Object, T>, int> = 0>
        T* getPrimaryObject()
        {
            return dynamic_cast<T*>(mPrimaryObject.get());
        }

        template <typename T, std::enable_if_t<std::is_base_of_v<Object, T>, int> = 0>
        T* findObjectByGuid(Guid guid)
        {
            auto findResult = mSecondaryObjects.find(guid);
            if (findResult != mSecondaryObjects.end())
                return dynamic_cast<T*>(findResult->second.get());
            return nullptr;
        }

    private:
        std::string mPath;
        osg::ref_ptr<Object> mPrimaryObject;
        std::unordered_map<Guid, osg::ref_ptr<Object>> mSecondaryObjects;
    };
}