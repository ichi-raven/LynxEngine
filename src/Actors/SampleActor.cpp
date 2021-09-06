
#include <Actors/SampleActor.hpp>

#include <Engine/Components/MeshComponent.hpp>
#include <Engine/Components/SkeletalMeshComponent.hpp>

#include <cassert>
#include <iostream>

#include <Engine/System/System.hpp>
#include <Engine/Application/ActorsInScene.hpp>

#include <Actors/SampleActor2.hpp>

#include <Engine/Components/MaterialComponent.hpp>
#include <Engine/Components/CameraComponent.hpp>
#include <Engine/Components/LightComponent.hpp>
#include <Engine/Components/SpriteComponent.hpp>
#include <Engine/Components/SoundComponent.hpp>

#include <Engine/Utility/Transform.hpp>

#include <Engine/Resources/Resources.hpp>

using namespace Cutlass;
using namespace Engine;

SampleActor::~SampleActor()
{

}

void SampleActor::awake()
{
	auto&& renderer = getSystem()->renderer;
	auto&& loader = getSystem()->loader;
	auto context = getContext();

	//コンポーネント追加
	mMesh 			= addComponent<Engine::SkeletalMeshComponent>();
	auto material 	= addComponent<Engine::MaterialComponent>();
	auto camera 	= addComponent<Engine::CameraComponent>();
	mLight 			= addComponent<Engine::LightComponent>();
	auto light2 	= addComponent<Engine::LightComponent>();
	auto sound 		= addComponent<Engine::SoundComponent>();
	mSprite 		= addComponent<Engine::SpriteComponent>();
	auto text 		= addComponent<Engine::TextComponent>();

	//ロード
	loader->load("../resources/models/CesiumMan/glTF/CesiumMan.gltf", mMesh, material);
	loader->load("font-test.png", mSprite);

	//sprite
	std::wstring str(L"ABC123");
	text.lock()->loadFont("../resources/fonts/NikkyouSans-mLKax.ttf");
	text.lock()->render(str, context, 256, 128);

	mSprite.lock()->getTransform().setPos(glm::vec3(300, 200, 0));
	mSprite.lock()->getTransform().setScale(glm::vec3(0.7f, 0.7f, 0));
	mSprite.lock()->setCenter();


	text.lock()->getTransform().setPos(glm::vec3(500, 200, 0));
	text.lock()->getTransform().setScale(glm::vec3(0.7f, 0.7f, 0));
	//text.lock()->setCenter();
	
	//skeletal mesh(CesiumMan)
	mMesh.lock()->setAnimationIndex(0);
	mMesh.lock()->getTransform().setPos(glm::vec3(0.f, 0.f, 0.6f));
	mMesh.lock()->changeDefaultAxis(glm::vec3(1.f, 0, 0), glm::vec3(0, 0, -1.f), glm::vec3(0, 1.f, 0));
	
	//camera
	camera.lock()->getTransform().setPos(glm::vec3(0, 2.f, 6.f));
	camera.lock()->setViewParam(glm::vec3(0, 0, -10.f), glm::vec3(0, 1.f, 0));
	camera.lock()->setProjectionParam(45.f, getCommonRegion()->width, getCommonRegion()->height, 0.1f, 1000.f);

	//light
	auto lightColor = glm::vec4(0.3f, 0.3f, 0.3f, 0);
	auto lightDir = glm::vec3(-1.f, -1.f, -1.f);
	mLight.lock()->setAsDirectionalLight(lightColor, lightDir);

	//renderer
	renderer->setCamera(camera);
	renderer->add(mLight);
	renderer->add(mMesh, material);
	//renderer->add(mSprite);
	renderer->add(text);

	//sound
	sound.lock()->create("../resources/sounds/06 - FromSoftware - Bravado.wav");
}

void SampleActor::init()
{
	//他アクタに関連する処理用,関数名はUnity準拠
	assert(getComponents<Engine::LightComponent>().value().size() == 2);
	assert(getActor<SampleActor2>("SampleActor2"));
}

void SampleActor::update()
{
	auto&& input 	= getSystem()->input;
	auto&& renderer = getSystem()->renderer;
	auto camera = getComponent<Engine::CameraComponent>().value();
	auto sound 	= getComponent<Engine::SoundComponent>().value();
	auto text 	= getComponent<Engine::TextComponent>().value();

	std::cerr << "now playing : " << sound.lock()->getPlayingTime() << "\n";

	auto context = getContext();
	//std::wstring str;
	std::wostringstream wss;
	wss << getCommonRegion()->fps;
	//str = wss.str();
	//std::wcout << L"Debug : " << str << "\n";
	text.lock()->render(wss.str(), context, 256, 128);

	{//input control
		constexpr float speed = 2.f;
		glm::vec3 vel(0.f);
		float rot = 0;
		auto& transform = text.lock()->getTransform();//mMesh.lock()->getTransform();

		if(input->getKey(Key::W))
			vel.z = -speed;
		if(input->getKey(Key::S))
			vel.z = speed;
		if(input->getKey(Key::A))
			vel.x = -speed;
		if(input->getKey(Key::D))
			vel.x = speed;
		if(input->getKey(Key::Up))
			vel.y = speed;
		if(input->getKey(Key::Down))
			vel.y = -speed;
		if(input->getKey(Key::Left))
			rot = -10;
		if(input->getKey(Key::Right))
			rot = 10;

		if(input->getKey(Key::Space))
			renderer->remove(mMesh);
		
		transform.setVel(vel);
		//transform.setRotAxis(glm::vec3(0, 1.f, 0));
		//transform.setRotAcc(rot);
		glm::vec3 pos = transform.getPos();

		if(input->getKeyDown(Key::P))
			sound.lock()->play();
		
		if(input->getKeyDown(Key::O))
			sound.lock()->stop();
		
	}

	if(getCommonRegion()->frame % 120 == 0)
	{
		srand(time(NULL));
		float random[] = {1.f * rand() / RAND_MAX, 1.f * rand() / RAND_MAX, 1.f * rand() / RAND_MAX};
		float random2[] = {1.f * (rand() % 10 + 1) - 5, 1.f * (rand() % 5 + 5), 1.f * (rand() % 10 + 1) - 5};
		auto lightDir = glm::vec3(random2[0], random2[1], random2[2]);
		auto lightColor = glm::vec4(random[0], random[1], random[2], 1.f);
		mLight.lock()->setAsDirectionalLight(mLight.lock()->getColor(), lightDir);
	}
}