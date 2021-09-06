#pragma once

#include <Engine/Application/Application.hpp>
#include "SceneCommonRegion.hpp"
#include "SceneList.hpp"

class SampleActor;

class TestScene : public Engine::Scene<SceneList, MyCommonRegion>
{
	GEN_SCENE(TestScene, SceneList, MyCommonRegion)

private:

	std::weak_ptr<SampleActor> mSampleActor;
};