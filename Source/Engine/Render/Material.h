#pragma once
#include "Shader.h"

namespace xxx
{
    class Material : public Object
    {
        using Super = Object;
    public:
        virtual void serialize(Serializer& serializer)
        {
            Super::serialize(serializer);

        }

    protected:
        osg::ref_ptr<Shader> mShader;
    };
}
