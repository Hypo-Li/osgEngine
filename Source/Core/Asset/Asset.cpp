#include "Asset.h"

namespace xxx
{


    size_t Asset::getReferenceIndex(const std::string& assetPath, std::vector<std::string>& reference)
    {
        size_t index;
        auto it = std::find(reference.begin(), reference.end(), assetPath);
        if (it == reference.end())
        {
            index = reference.size();
            reference.push_back(assetPath);
        }
        else
        {
            index = std::distance(reference.begin(), it);
        }
        return index;
    }

}
