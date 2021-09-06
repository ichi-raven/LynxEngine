#include <Scenes/SceneList.hpp>
#include <Scenes/SceneCommonRegion.hpp>
#include <Scenes/TestScene.hpp>

#include <Engine/Application/Application.hpp>

#include <Cutlass/Cutlass.hpp>

#include <iostream>
#include <array>
#include <chrono>
#include <numeric>

int main()
{
	//定数
	constexpr bool debug 				= true;
	constexpr bool fullscreen 			= false;
	constexpr bool vsync 				= true;
	constexpr uint16_t windowWidth 		= 1000;
	constexpr uint16_t windowHeight 	= 800;
	constexpr uint16_t frameBufferNum 	= 3;
	constexpr char* applicationName 	= "LynxEngineApp";
	constexpr char* windowName 			= "LynxEngineWindow";


	//アプリケーション実体作成
	Engine::Application<SceneList, MyCommonRegion> app
	(
		applicationName, debug,
		{Cutlass::WindowInfo(windowWidth, windowHeight, frameBufferNum, windowName, fullscreen, vsync)}
	);
	
	//共有情報セット
	app.mCommonRegion->width 			= windowWidth;
	app.mCommonRegion->height 			= windowHeight;
	app.mCommonRegion->frameBufferNum 	= frameBufferNum;
	app.mCommonRegion->frame			= 0;
	app.mCommonRegion->fps 				= 0;

	//シーン追加
	app.addScene<TestScene>(SceneList::eTest);
	
	//このシーンから開始, Scene::initを実行
	app.init(SceneList::eTest);

	std::array<float, 10> times;//10f平均でfpsを計測
	std::chrono::high_resolution_clock::time_point now, prev = std::chrono::high_resolution_clock::now();

	//メインループ
	while (!app.endAll())
	{
		{//フレーム数, fps
			now = std::chrono::high_resolution_clock::now();
			times[app.mCommonRegion->frame % 10] = app.mCommonRegion->deltatime = std::chrono::duration_cast<std::chrono::microseconds>(now - prev).count() / 1000000.;
			//std::cerr << "now frame : " << app.mCommonRegion->frame << "\n";
			app.mCommonRegion->fps = 1. / (std::accumulate(times.begin(), times.end(), 0.) / 10.);
			//std::cerr << "FPS : " << << "\n";
		}

		//アプリケーション更新
		app.update();

		{//フレーム, 時刻更新
			++app.mCommonRegion->frame;
			prev = now;
		}
	}

	return 0;
}