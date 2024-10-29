#pragma once
#include <string>
#include <cstring>

#if 0
namespace xxx
{
    class StringHashPool
    {

    };

    class StringHash
    {
    public:
        StringHash(const char* str)
        {
            size_t len = std::strlen(str);
            _hash = calculateCityHash64(str, len);
#ifdef _DEBUG
            const char* collided = checkHashCollision(_hash, str, len);
            if (collided)
            {
                // LOG_ERROR: Need to change hash seed!
            }
#endif // _DEBUG

        }

    private:
        uint64_t _hash;
#ifdef _DEBUG
        const char* _str;
#endif // _DEBUG

        static uint64_t calculateCityHash64(const char* str, size_t len);

        static const char* checkHashCollision(uint64_t hash, const char* str, size_t len);

        static void storeStringHash(uint64_t hash, const char* str, size_t len);

        static uint64_t _sSeed0, _sSeed1;
    };
}

#endif // 0
