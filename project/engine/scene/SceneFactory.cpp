#include "SceneFactory.h"
#include "TitleScene.h"
#include "GamePlayScene.h"

BaseScene* SceneFactory::CreateScene(const std::string& sceneName)
{
    // 次のシーンを生成
	BaseScene* nextScene = nullptr;

	if(sceneName == "TitleScene")
	{
		nextScene = new TitleScene();
	}
	else if(sceneName == "GamePlayScene")
	{
		nextScene = new GamePlayScene();
	}

	return nextScene;
}
