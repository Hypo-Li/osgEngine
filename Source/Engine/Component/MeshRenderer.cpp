#include "MeshRenderer.h"

namespace xxx
{
    class LODExt : public osg::LOD
    {
        float _mainCameraRange = 0.0f;
    public:
        virtual void traverse(osg::NodeVisitor& nv) override
        {
            switch (nv.getTraversalMode())
            {
            case(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN):
                std::for_each(_children.begin(), _children.end(), osg::NodeAcceptOp(nv));
                break;
            case(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN):
            {
                float required_range = 0;
                if (_rangeMode == DISTANCE_FROM_EYE_POINT)
                {
                    required_range = nv.getDistanceToViewPoint(getCenter(), true);
                }
                else
                {
                    osg::CullStack* cullStack = nv.asCullStack();
                    if (cullStack && cullStack->getLODScale())
                    {
                        float boundSphereD = cullStack->clampedPixelSize(getBound());
                        required_range = boundSphereD / cullStack->getLODScale();
                        float boundSphereR = boundSphereD * 0.5;
                        float boundSphereArea = boundSphereR * boundSphereR * osg::PIf;
                        osg::Viewport* viewport = cullStack->getViewport();
                        float screenPixelCount = viewport->width() * viewport->height();
                        float pixelRatio = std::sqrt(boundSphereArea / screenPixelCount);
                        required_range = std::min(pixelRatio, 0.9999999f);
                    }
                    else
                    {
                        // fallback to selecting the highest res tile by
                        // finding out the max range
                        for (unsigned int i = 0; i < _rangeList.size(); ++i)
                        {
                            required_range = osg::maximum(required_range, _rangeList[i].first);
                        }
                    }
                }

                osgUtil::CullVisitor* cullVisitor = nv.asCullVisitor();
                if (cullVisitor)
                {
                    if (cullVisitor->getCullMask() == SHADOW_CAST_MASK)
                        required_range = _mainCameraRange;
                    else
                        _mainCameraRange = required_range;
                }

                unsigned int numChildren = _children.size();
                if (_rangeList.size() < numChildren) numChildren = _rangeList.size();

                for (unsigned int i = 0; i < numChildren; ++i)
                {
                    if (_rangeList[i].first <= _mainCameraRange && _mainCameraRange < _rangeList[i].second)
                    {
                        _children[i]->accept(nv);
                    }
                }
                break;
            }
            default:
                break;
            }
        }
    };

    class CullCallback : public osg::NodeCallback
    {
    public:
        CullCallback(osg::Uniform* uniform) : mPrevFrameMVPMatrixUniform(uniform) {}

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            osgUtil::CullVisitor* cullVisitor = nv->asCullVisitor();
            mPrevFrameMVPMatrixUniform->set(mPrevFrameMVPMatrix);
            uint32_t frameNumberMod8 = cullVisitor->getFrameStamp()->getFrameNumber() % 8;
            double sampleX = halton(frameNumberMod8, 2) - 0.5, sampleY = halton(frameNumberMod8, 3) - 0.5;
            osg::Matrixf projectionMatrix = cullVisitor->getCurrentCamera()->getProjectionMatrix();
            const osg::Viewport* viewport = cullVisitor->getCurrentCamera()->getViewport();
            projectionMatrix(2, 0) = 2 * sampleX / viewport->width();
            projectionMatrix(2, 1) = 2 * sampleY / viewport->height();
            mPrevFrameMVPMatrix = (*cullVisitor->getModelViewMatrix()) * projectionMatrix;
            traverse(node, nv);
        }

    private:
        osg::ref_ptr<osg::Uniform> mPrevFrameMVPMatrixUniform;
        osg::Matrixf mPrevFrameMVPMatrix;

        static double halton(int index, int base)
        {
            double result = 0.0;
            double f = 1.0 / base;
            int i = index;
            while (i > 0)
            {
                result = result + f * (i % base);
                i = i / base;
                f = f / base;
            }
            return result;
        }
    };

    MeshRenderer::MeshRenderer() :
        mOsgLOD(new LODExt)
    {
        mOsgComponentGroup->addChild(mOsgLOD);
        mOsgLOD->setRangeMode(osg::LOD::PIXEL_SIZE_ON_SCREEN);

        osg::ref_ptr<osg::Uniform> prevFrameMVPMatrixUniform = new osg::Uniform(osg::Uniform::FLOAT_MAT4, "uPrevFrameMVPMatrix");
        mOsgLOD->getOrCreateStateSet()->addUniform(prevFrameMVPMatrixUniform);
        mOsgLOD->addCullCallback(new CullCallback(prevFrameMVPMatrixUniform));
    }
}
