#pragma once 

#include "../Components/IComponent.hpp"

#include "../Utility/Transform.hpp"

#include <Cutlass/Cutlass.hpp>

namespace Lynx
{
    class SpriteComponent : public IComponent
    {
    public:

        struct Vertex
        {
            glm::vec3 pos;
            glm::vec2 uv;
        };

        struct Sprite
        {
            std::vector<Cutlass::HTexture> handles;
        };

        SpriteComponent();
        virtual ~SpriteComponent();

        void setTransform(const Transform& transform);

        Transform& getTransform();

        void setVisible(bool flag);
        bool getVisible() const;

        void setEnable(bool flag);
        bool getEnable() const;

        void setCenter();
        void setUpperLeft();
        bool getCenterFlag() const;

        void create(const Sprite& sprite);

        const Cutlass::HTexture& getSpriteTexture() const;

        virtual void update();

    protected:
        Transform mTransform;
        Sprite mSprite;
        size_t mIndex;
        bool mCenter;
        bool mEnabled;
        bool mVisible;
    };
}