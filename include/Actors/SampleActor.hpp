#pragma once

#include "../Scenes/SceneCommonRegion.hpp"

#include "../Engine/Actors/IActor.hpp"

namespace Engine
{
    class SkeletalMeshComponent;
    class LightComponent;
    class SpriteComponent;
}

class SampleActor : public Engine::IActor<MyCommonRegion>
{
    GEN_ACTOR(SampleActor, MyCommonRegion)

private:
    std::weak_ptr<Engine::SkeletalMeshComponent> mMesh;
    std::weak_ptr<Engine::LightComponent> mLight;
    std::weak_ptr<Engine::SpriteComponent> mSprite;
};