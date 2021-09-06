#include <Engine/Components/SpriteComponent.hpp>

namespace Engine
{
    SpriteComponent::SpriteComponent()
    : mIndex(0)
    , mEnabled(true)
    , mVisible(true)
    , mCenter(false)
    {

    }

    SpriteComponent::~SpriteComponent()
    {

    }

    void SpriteComponent::setTransform(const Transform& transform)
    {
        mTransform = transform;
    }

    Transform& SpriteComponent::getTransform()
    {
        return mTransform;
    }

    void SpriteComponent::setVisible(bool flag)
    {
        mVisible = flag;
    }

    bool SpriteComponent::getVisible() const
    {
        return mVisible;
    }

    void SpriteComponent::setEnable(bool flag)
    {
        mEnabled = flag;
    }

    bool SpriteComponent::getEnable() const
    {
        return mEnabled;
    }

    void SpriteComponent::setCenter()
    {
        mCenter = true;
    }

    void SpriteComponent::setUpperLeft()
    {
        mCenter = false;
    }

    bool SpriteComponent::getCenterFlag() const
    {
        return mCenter;
    }

    void SpriteComponent::create(const Sprite& sprite)
    {
        mSprite = sprite;
    }

    const Cutlass::HTexture& SpriteComponent::getSpriteTexture() const
    {
        if(mSprite.handles.size() == 0)
            assert(!"sprite is not created!");

        return mSprite.handles[mIndex];
    }

    void SpriteComponent::update()
    {
        if(!mEnabled)
            return;
        mTransform.update();
        mIndex = (1 + mIndex) % mSprite.handles.size();
    }

}