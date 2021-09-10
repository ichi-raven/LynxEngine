#include <Lynx/Components/LightComponent.hpp>

namespace Lynx
{
    LightComponent::LightComponent()
    : mEnable(true)
    {
        //とりあえず
        // DirectionalLightParam data;
        mColor = glm::vec4(0.9f, 0.9f, 0.9f, 1.f);
        mRange = 1.f;
        mDirection = glm::vec3(0);//glm::vec3(0, 0.7071f, 0.7071f);
        //setAsDirectionalLight(data);
    }

    void LightComponent::setAsPointLight(const glm::vec4& color, float range)
    {
        mType = LightType::ePointLight;
        mColor = color;
        mRange = range;
        mDirection = glm::vec3(0);
    }

    void LightComponent::setAsDirectionalLight(const glm::vec4& color, const glm::vec3& direction)
    {
        mType = LightType::eDirectionalLight;
        mColor = color;
        mDirection = glm::normalize(direction);
    }

    const LightComponent::LightType LightComponent::getType() const
    {
        return mType;
    }

    const glm::vec4& LightComponent::getColor() const
    {
        return mColor;
    }

    const glm::vec3& LightComponent::getDirection() const
    {
        return mDirection;
    }

    const float LightComponent::getRange() const
    {
        return mRange;
    }

    void LightComponent::setEnable(bool flag)
    {
        mEnable = flag;
    }

    void LightComponent::setEnable()
    {
        mEnable = !mEnable;
    }

    const bool LightComponent::getEnable() const
    {
        return mEnable;
    }

    void LightComponent::setTransform(const Transform& transform)
    {
        mTransform = transform;
    }

    Transform& LightComponent::getTransform()
    {
        return mTransform;
    }

    const Transform& LightComponent::getTransform() const
    {
        return mTransform;
    }

    void LightComponent::update()
    {
        mTransform.update();
    }
}