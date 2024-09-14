#include "AssetSaver.h"
#include "../Asset.h"

namespace xxx
{
    using namespace refl;
    void AssetSaver::serialize(std::string* value, size_t count)
    {
        /*std::vector<uint32_t> stringIndices(count);
        for (size_t i = 0; i < count; ++i)
        {
            auto findResult = std::find(mAsset->mStringTable.begin(), mAsset->mStringTable.end(), value[i]);
            if (findResult == mAsset->mStringTable.end())
            {
                mAsset->mStringTable.emplace_back(value[i]);
                stringIndices[i] = mAsset->mStringTable.size() - 1;
            }
            else
            {
                stringIndices[i] = findResult - mAsset->mStringTable.begin();
            }
        }
        serialize(stringIndices.data(), count);*/
    }
}
