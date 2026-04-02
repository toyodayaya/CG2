#include "SceneManager.h"
#include <cassert>

SceneManager* SceneManager::instance = nullptr;

SceneManager* SceneManager::GetInstance()
{
	if (instance == nullptr)
	{
		instance = new SceneManager;
	}
	return instance;
}

void SceneManager::ChangeScene(const std::string& sceneName)
{
	assert(sceneFactory_);
	assert(nextScene_ == nullptr);

	// 次のシーンを生成
	nextScene_ = sceneFactory_->CreateScene(sceneName);
}

void SceneManager::Update()
{
	// シーン切り替え

	// 次シーンの予約があったら
	if (nextScene_)
	{
		// 旧シーンを終了する
		if (scene_)
		{
			scene_->Finalize();
			delete scene_;
		}

		// シーンを切り替える
		scene_ = nextScene_;
		nextScene_ = nullptr;

		// 次シーンを初期化する
		scene_->Initialize();
	}

	// 実行中シーンを更新する
	scene_->Update();
}

void SceneManager::Draw()
{
	// 実行中シーンを描画する
	scene_->Draw();
}


void SceneManager::Finalize()
{
	// 最後のシーンの終了と解放
	scene_->Finalize();
	delete scene_;
	delete instance;
	instance = nullptr;
}
