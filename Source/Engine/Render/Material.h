#pragma once
#include "Shader.h"

namespace xxx
{
    class Material : public Object
    {
        using Super = Object;
    public:

    protected:
        osg::ref_ptr<Shader> mShader;
    };
}
